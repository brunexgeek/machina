#include <sys/bcm2837.h>
#include <sys/uart.h>
#include <sys/log.h>
#include <mc/stdio.h>
#include <mc/stdlib.h>
#include <mc/string.h>
#include <sys/pmm.hh>
#include <sys/display.hh>
#include <sys/heap.h>
#include <sys/errors.h>
#include <sys/procfs.h>
#include <sys/ramdsk.h>
#include <sys/mailbox.h>
#include <sys/device.hh>

/*
 * Pointer to the first C/C++ static construtor.
 */
extern void (*__begin_init_array) (void);

/*
 * Pointer to the end of C/C++ static construtor list.
 * The last valid constructor is the one before this.
 */
extern void (*__end_init_array) (void);

static int proc_sysname( uint8_t *buffer, int size, void *data )
{
	(void) data;

	const char *sysname = "Machina";
	size_t len = strlen(sysname);

	if (size >= (int)(len + 1) * 2)
	{
		strncpy((char*) buffer, sysname, len + 1);
		return (int)(len + 1) * 2;
	}

	return 0;
}

void kernel_panic( const char *path, int line )
{
	klog_print("KERNEL PANIC!   at %s:%d\n", path, line);
	while (true) asm("wfi");
}

void kernel_print_file( const char *path )
{
	file_t *fp = NULL;
	if (vfs_open(path, 0, &fp) == EOK)
	{
		char buf[1024];
		int c = vfs_read(fp, (uint8_t*)buf, sizeof(buf));
		if (c >= 0)
		{
			c /= sizeof(char);
			buf[c] = 0;
			klog_print(buf);
		}
		vfs_close(fp);
	}
}

extern SOC_INFO kvar_soc_info;

extern "C" void kernel_main()
{
    uart_init();
	// call C++ static construtors
	void (**p) (void) = 0;
	for (p = &__begin_init_array; p < &__end_init_array; ++p) (**p) ();

	klog_print("Raspberry PI 3\n  Processor: ARMv8 aarch64 %d cores\n", kvar_soc_info.info.cores_enabled);

	struct mailbox_message message;
	if (mailbox_tag(MAILBOX_TAG_GET_ARM_MEMORY, &message) == 0)
		klog_print("     Memory: %d MB\n", message.tag.memory.size / 1024 / 1024);
	if (mailbox_tag(MAILBOX_TAG_GET_BOARD_MAC_ADDRESS, &message) == 0)
		klog_print("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
			message.tag.mac.address[0],
			message.tag.mac.address[1],
			message.tag.mac.address[2],
			message.tag.mac.address[3],
			message.tag.mac.address[4],
			message.tag.mac.address[5]);
	if (mailbox_tag(MAILBOX_TAG_GET_BOARD_SERIAL, &message) == 0)
		klog_print("     Serial: %lu\n", message.tag.serial.value);

	// Note: The next calls will initialize our memory managers. It's
	//       crucial that no dynamic memory allocation is attempted before
	//       that point. This includes using 'new' and 'delete' C++ operators.
    pmm_initialize();
	heap_initialize();

    procfs_initialize();
	procfs_register("/sysname", proc_sysname, NULL);
	if (vfs_mount("procfs", nullptr, "/proc", "", 0, 0) == EOK)
		klog_print("Mounted '/proc'\n");

	pmm_register();
	heap_register();

    kernel_print_file("/proc/frames");
    kernel_print_file("/proc/heap");

	kdev_initialize();
	device_t *ramdsk = nullptr;
	kramdsk_create_device(nullptr, 10 * 1024 * 1024, &ramdsk);
	kdev_enumerate();


	klog_print("Done!\n");
	while (true) { asm("wfi"); };
}