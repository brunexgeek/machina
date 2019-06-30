#include <machina/VMM.hh>
#include <sys/system.h>
#include <sys/soc.h>
#include <machina/PMM.hh>
#include <sys/mailbox.hh>
#include <sys/uart.hh>


#define COARSE_BASE_ADDRESS(x) \
	( (x) & 0xFFFFFC00 )

#define COARSE_DOMAIN(x) \
	( (x) & 0x01E0 )


#define VMM_flushTLB() \
	asm volatile ("mcr p15, 0, %0, c8, c7, 0" : : "r" (0) : "memory")


#define MEMORY_KB(x)         ( (x) * 1024 )
#define MEMORY_MB(x)         ( (x) * 1024 * 1024 )


#define VMM_DP_MANAGER             (0x03)  // 0b11 = access is uncontrolled, no permission aborts generated
#define VMM_DP_RESERVED            (0x04)  // 0b10 = unpredictable
#define VMM_DP_CLIENT              (0x01)  // 0b01 = access controlled by permission values set in PTE
#define VMM_DP_NO_ACCESS           (0x00)  // 0b00 = generates a domain fault


#define VMM_AP_RW_RW               (0x03)  // 0b11 = kernel rw, user rw
#define VMM_AP_RW_RO               (0x02)  // 0b10 = kernel rw, user ro
#define VMM_AP_RW_NA               (0x01)  // 0b01 = kernel rw, user no access
#define VMM_AP_NA_NA               (0x00)  // 0b00 = no access


/**
 * @brief Master/section page table (L1).
 *
 * The dynamically allocated memory pointed here must be 16KB
 * aligned.
 */
volatile machina::SectionDescriptor *kernelL1;

/**
 * @brief Coarse page table (L2).
 *
 * We are using coarse page table for L2 supposing we have 4 KB
 * pages (check the value at @ref SYS_PAGE_SIZE).
 */
static uint32_t *kernelL2;


static machina::VMM instance;


namespace machina {


VMM::VMM()
{
	// nothing to do
}


VMM::~VMM()
{
	// nothing to do
}


VMM &VMM::getInstance()
{
	return instance;
}


void VMM::initialize()
{
	kernelL1 = (SectionDescriptor*) PMM::getInstance().allocate(
		MEMORY_KB(16) / SYS_PAGE_SIZE, 16, PFT_PTABLE );
	// probe the GPU memory map
	MemoryTag gpuSplit;
	mailbox_getProperty(MAILBOX_CHANNEL_ARM, 0x00010006, &gpuSplit, sizeof(gpuSplit));
	// probe the ARM memory map
	MemoryTag armSplit;
	mailbox_getProperty(MAILBOX_CHANNEL_ARM, 0x00010005, &armSplit, sizeof(armSplit));
	// compute the region beyond valid memory
	size_t begin = (gpuSplit.base + gpuSplit.size);
	if (armSplit.base > gpuSplit.base)
		begin = (armSplit.base + armSplit.size);
	begin = (begin + MEMORY_MB(1) - 1) & ~(MEMORY_MB(1) - 1);
	begin /= MEMORY_MB(1);

	// set the domain #0 for kernel pages
	setDomainPerm(0, VMM_DP_CLIENT);
	// set the domain #1 for heap pages
	setDomainPerm(1, VMM_DP_CLIENT);
	// set the domain #2 for invalid pages (no access)
	setDomainPerm(2, VMM_DP_CLIENT);

	// set permissions for kernel pages (read only in user mode)
	for (size_t i = 0; i < SYS_HEAP_START / MEMORY_MB(1); ++i)
	{
		*((uint32_t*) kernelL1 + i) = 0;
		kernelL1[i].base = i & 0xFFF;
		kernelL1[i].AP10 = VMM_AP_RW_RO;
		kernelL1[i].type = 0x02;
	}
	// set permissions for heap pages
	for (size_t i = SYS_HEAP_START / MEMORY_MB(1); i < begin; ++i)
	{
		*((uint32_t*) kernelL1 + i) = 0;
		kernelL1[i].base = i & 0xFFF;
		kernelL1[i].AP10 = VMM_AP_RW_RO;
		kernelL1[i].domain = 1;
		kernelL1[i].type = 0x02;
	}

	// TODO: use smaller page for video memory because its pages are 4KB

	// mark invalid pages (those that maps to invalid physical addresses) with no access
	for (size_t i = begin; i < 4096; ++i)
	{
		*((uint32_t*) kernelL1 + i) = 0;
		kernelL1[i].base = i & 0xFFF;
		kernelL1[i].AP10 = VMM_AP_RW_RO;
		kernelL1[i].domain = 2;
		kernelL1[i].type = 0x02;
	}

	// tell to VMM the location of our L1 table
	asm volatile (
		"mcr         p15, 0, %0, c2, c0, 0;"
		:: "r"(kernelL1) );

	// enable VMM
	asm volatile (
		"mrc         p15, 0, r0, c1, c0, 0;"
		"orr         r0, r0, #0x1;"
		"mcr         p15, 0, r0, c1, c0, 0;"
		::: "r0" );
}


void VMM::setDomainPerm(
	size_t domain,
	size_t perm )
{
	size_t shift = (domain & 0x0F) * 2;

	size_t value;
	asm volatile ("mrc p15, 0, %0, c3, c0, 0" : "=r" (value) :: "memory");
	value &= ~(0x03 << shift);
	value |= perm << shift;
	asm volatile ("mcr p15, 0, %0, c3, c0, 0" :: "r" (value) : "memory");
}


void VMM::printL1()
{
	size_t domain = 0;
	size_t perm = VMM_DP_CLIENT;
	size_t shift = (domain & 0x0F) * 2;

	size_t value;
	asm volatile ("mrc p15, 0, %0, c3, c0, 0" : "=r" (value) :: "memory");
	value &= ~(0x03 << shift);
	value |= perm << shift;
	uart_print(u"%08x\n", value);

	for (size_t i = 0; i < 4; ++i)
		uart_print(u"Entry %d = %08x\n", i, *((uint32_t*)kernelL1 + i));
}


PageTable *VmCreatePageTable()
{
	PMM &phys = PMM::getInstance();

	// computes the amount of frames required to map
	// the entire memory
	size_t size = sizeof(uint32_t) * 4096;
	//size += phys.get() * 1024;

	return 0;
}


}