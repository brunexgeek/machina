/*
 * Specify the binary layout for kernel image.
 */

SECTIONS
{
	/*
	 * The third stage bootloader (start.elf) loads the kernel image
	 * at 0x8000 and runs it.
	 */
	.text 0x8000:
	{
		/*
		 * Kernel entry point function (written in assembly) which
		 * calls the kernel main function (written in C).
		 */
		*(.entryPoint)
		*(.text*)
	}

	_end_text = .;

	__begin_init_array = .;

	/*
	 * DT_INIT_ARRAY section containing the list of functions addresses
	 * that must be called, in-order, to perform initialization. This
	 * is required because we are using C++ objects.
	 *
	 * http://www.netmite.com/android/mydroid/1.6/bionic/linker/README.TXT
	 */
	.init_array :
	{
		*(.init_array*)
	}

	__end_init_array = .;

	.rodata :
	{
		*(.rodata*)
	}

	.data :
	{
		*(.data*)
	}

	__begin_bss = .;

	.bss :
	{
		*(.bss*)
	}

	__end_bss = .;

	__kernel_end = .;
}