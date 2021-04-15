/*
 * Specify the binary layout for kernel image.
 */

SECTIONS
{

	/*
	 * The RPI third stage bootloader (start.elf) loads the kernel image
	 * at 0x8000 (0x80000 for aarch64) and runs it.
	 */
	.text 0x80000:
	{
		. = ALIGN(4);
		_kernel_begin = .;
		/*
		 * Kernel entry point function (written in assembly) which
		 * calls the kernel main function (written in C).
		 */
		*(.text.entry)
		*(.text*)
	}

	_end_text = .;


	/*
	 * DT_INIT_ARRAY section containing the list of functions addresses
	 * that must be called, in-order, to perform initialization. This
	 * is required because we are using C++ objects.
	 *
	 * http://www.netmite.com/android/mydroid/1.6/bionic/linker/README.TXT
	 */
	.init_array :
	{
		. = ALIGN(4);
		__begin_init_array = .;
		*(.init_array*)
		__end_init_array = .;
	}


	.rodata :
	{
		. = ALIGN(4);
		*(.rodata*)
	}

	.data :
	{
		. = ALIGN(4);
		*(.data*)
	}

	.bss :
	{
		. = ALIGN(4);
		__begin_bss = .;
		*(.bss*)
		__end_bss = .;
		__size_bss = __end_bss - __begin_bss;
	}

	.stack_core0 :
	{
        . = ALIGN(16);				/* Stack must always be aligned to 16 byte boundary AAPCS64 call standard */
        __stack_start_core0__ = .;
        . = . + (1024 * 1);				/* EL0 stack size */
        __EL0_stack_core0 = .;
        . = . + (1024 * 16);				/* EL1 stack size */
        __EL1_stack_core0 = .;
        . = . + (1024 * 16);				/* EL2 stack size (start-up) */
        __EL2_stack_core0 = .;
        __stack_end_core0__ = .;
    }

	.stack_core1 :
	{
        . = ALIGN(16);				/* Stack must always be aligned to 16 byte boundary AAPCS64 call standard */
        __stack_start_core1__ = .;
        . = . + 512;				/* EL0 stack size */
        __EL0_stack_core1 = .;
		. = . + 1024;				/* EL1 stack size */
        __EL1_stack_core1 = .;
        . = . + 512;				/* EL2 stack size (start-up) */
        __EL2_stack_core1 = .;
        __stack_end_core1__ = .;
    }

	.stack_core2 :
	{
        . = ALIGN(16);				 /* Stack must always be aligned to 16 byte boundary AAPCS call standard */
        __stack_start_core2__ = .;
        . = . + 512;				/* EL0 stack size */
        __EL0_stack_core2 = .;
        . = . + 1024;				/* EL1 stack size */
        __EL1_stack_core2 = .;
        . = . + 512;				/* EL2 stack size (start-up) */
        __EL2_stack_core2 = .;
        __stack_end_core2__ = .;
    }

	.stack_core3 :
	{
        . = ALIGN(16);				/* Stack must always be aligned to 16 byte boundary AAPCS call standard */
        __stack_start_core3__ = .;
        . = . + 512;				/* EL0 stack size */
        __EL0_stack_core3 = .;
        . = . + 1024;				/* EL1 stack size */
        __EL1_stack_core3 = .;
        . = . + 512;				/* EL2 stack size (start-up) */
       	__EL2_stack_core3 = .;
       	__stack_end_core3__ = .;
    }

	_kernel_end = .;
	_kernel_size = _kernel_end - _kernel_begin;
}

