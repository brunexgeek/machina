#include <sys/system.h>
#include <sys/sysio.hh>
#include <sys/errors.h>
#include <sys/soc.h>
#include <sys/types.h>
#include <sys/sync.h>
#include <sys/Timer.hh>
#include <sys/PhysicalMemory.hh>
#include <sys/Memory.hh>
#include <sys/Display.hh>
#include <sys/Screen.hh>
#include <sys/Mailbox.hh>


/*
 * BSS area begin marker.
 */
extern uint8_t __begin_bss;

/*
 * BSS area end marker.
 */
extern uint8_t __end_bss;

/*
 * Pointer to the first C/C++ static construtor.
 */
extern void (*__begin_init_array) (void);

/*
 * Pointer to the end of C/C++ static construtor list.
 * The last valid constructor is the one before this.
 */
extern void (*__end_init_array) (void);


namespace machina {


static volatile bool DISABLED_CORES[SYS_CPU_CORES];


struct MacAddressProperty
{
	machina::MailboxTag tag;
	uint8_t address[6];
	uint8_t padding[2];
};


int kernel_main()
{
	Display &display = Display::getInstance();
	//display.draw('B', 0, 0, Font::getMonospaceFont(), 0xffff, 0x0000);
	//display.draw('A', 30, 0, Font::getMonospaceFont(), 0xffff, 0x0000);

	TextScreen *ts = TextScreen::create(display.getWidth(), display.getHeight(), display.getDepth());
	PhysicalMemory::getInstance().print(*ts);
	//ts->refresh();

	void *ptr = machina::Memory::getInstance().allocate(4 * 1024 * 1024);

	machina::Memory::getInstance().print(*ts);
	ts->refresh();

	display.draw(*ts);

	do
	{
		asm volatile ("wfi");
	} while (true);
/*
	machina::PhysicalMemory phys;
	phys.print(display);
	size_t index = phys.allocate(3);
	phys.print(display);
	phys.free(index + 1, 1);
	phys.print(display);

	MemoryTag split;

	Mailbox::getProperty(MAILBOX_CHANNEL_ARM, 0x00010005, &split, sizeof(split));
	display.print("CPU memory start at ");
	display.printHex(split.base);
	display.print(" with ");
	display.printHex(split.size);
	display.print(" bytes\n");

	Mailbox::getProperty(MAILBOX_CHANNEL_ARM, 0x00010006, &split, sizeof(split));
	display.print("GPU memory start at ");
	display.printHex(split.base);
	display.print(" with ");
	display.printHex(split.size);
	display.print(" bytes\n");

	MacAddressProperty bla;
	machina::Mailbox::getProperty(MAILBOX_CHANNEL_ARM, 0x00010003, &bla, sizeof(bla));
	display.print("This board MAC address is: ");

	for (size_t i = 0; i < sizeof(bla.address); ++i)
	{
		display.print( HEXS[bla.address[i] >> 4] );
		display.print( HEXS[bla.address[i] & 0x0F] );
		if (i + 1 < sizeof(bla.address))
			display.print(':');
	}
	display.refresh();*/

	do
	{
		asm volatile ("wfi");
	} while (true);
	return 0;
};


/*
 * Raspberry Pi 1 uses ARMv6 that do not provides WFI instruction,
 * so we need to use the ARMv5 alternative. See the link
 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka13332.html
 * for more information.
 */
static void system_disableCore()
{
	while (true)
	{
		#if (RPIGEN == 1)

		size_t value = 0;
		asm volatile ("mcr p15, 0, %0, c7, c0, 4" : "=r" (value));

		#else

		asm volatile ("dsb");
		asm volatile ("wfi");

		#endif
	}
}


static void system_shutdown(void)
{
	#ifdef ENABLE_MULTI_CORE

	// read the CPU ID register to find out what core is shutting down
	size_t value = 0;
	asm volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r" (value));
	uint32_t current = value & (SYS_CPU_CORES - 1);
	// we don't shutdown the main core before to shutdown the others
	while (current == 0)
	{
		sync_dataMemBarrier();

		// check whether the other cores are disabled
		bool result = true;
		for (size_t i = 1; i < SYS_CPU_CORES; ++i)
			result &= DISABLED_CORES[i];

		if (!result)
		{
			// wait some time until next verification
			sync_dataSyncBarrier();
			asm volatile ("wfi");
		}
	}

	DISABLED_CORES[current] = true;
	sync_dataMemBarrier();

	#endif

	sync_disableInterrupts();

	#if (RPIGEN > 1)
	sync_dataSyncBarrier();
	#endif
	system_disableCore();
}

/**
 * @brief Reboots the Raspberry Pi.
 *
 * This code was originally implemented by PlutoniumBob and published
 * at https://www.raspberrypi.org/forums/viewtopic.php?f=72&t=53862#p413949
 */
static void system_reboot(void)
{
   sync_dataSyncBarrier();

   PUT32(SOC_PM_WDOG, SOC_PM_PASSWORD | 1);
   PUT32(SOC_PM_RSTC, SOC_PM_PASSWORD | SOC_PM_RSTC_WRCFG_FULL_RESET);

   system_disableCore();
}


static void system_initializeVFP (void)
{
	// Coprocessor Access Control Register
	unsigned cacr;
	asm volatile ("mrc p15, 0, %0, c1, c0, 2" : "=r" (cacr));
	cacr |= 3 << 20;	// cp10 (single precision)
	cacr |= 3 << 22;	// cp11 (double precision)
	asm volatile ("mcr p15, 0, %0, c1, c0, 2" : : "r" (cacr));
	sync_instSyncBarrier();

#define VFP_FPEXC_EN	(1 << 30)
	asm volatile ("fmxr fpexc, %0" : : "r" (VFP_FPEXC_EN));

	asm volatile ("fmxr fpscr, %0" : : "r" (0));
}


extern "C" void system_initialize()
{
	for (size_t i = 0; i < SYS_CPU_CORES; ++i)
		DISABLED_CORES[i] = false;

#if (RPIGEN != 1)
#ifdef ENABLE_MULTI_CORE
	// put all other CPU cores to sleep
	for (unsigned i = 1; i < SYS_CPU_CORES; i++)
	{
		// https://www.raspberrypi.org/forums/viewtopic.php?f=72&t=98904&start=25#p700528
		PUT32(0x4000008C + 0x10 * i, (uint32_t) &system_disableCore);
	}
#endif
#endif

	// initializes the VFP
	system_initializeVFP();

	// we don't have a library loader to clear the BBS area,
	// so we need to do that manually
	for (uint8_t *p = &__begin_bss; p < &__end_bss; ++p) *p = 0;

	// Note: The next loop will initialize our physical memory manager. It's
	//       crucial that no dynamic memory allocation is attempted before
	//       that point. The the 'new' and 'delete' C++ operators use the
	//       physical memory manager.

	// we also need to call the C/C++ static construtors manually
	void (**p) (void) = 0;
	for (p = &__begin_init_array; p < &__end_init_array; ++p) (**p) ();

	switch ( kernel_main () )
	{
		case EREBOOT:
			system_reboot();
			break;
		default:
			system_shutdown();
	}
}

#ifdef ENABLE_MULTI_CORE

extern "C" void system_enableCoreEx (void)
{
	// L1 data cache may contain random entries after reset, delete them
	InvalidateDataCacheL1Only ();

	system_initializeVFP();

	main_secondary();

	system_shutdown();
}

#endif


} // machina