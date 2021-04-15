extern "C" void kernel_schedule (void)
{
}

/*
 *	This is the TICK interrupt service routine, note. no SAVE/RESTORE_CONTEXT here
 *	as thats done in the bottom-half of the ISR in assembler.
 */
extern "C" void kernel_tick_isr(void)
{
}
