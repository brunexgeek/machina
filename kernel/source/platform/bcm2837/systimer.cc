#include <sys/timer.hh>
#include <sys/sync.h>


#define BCM2835_COMPARER_1 (1 << 1)
#define BCM2835_COMPARER_3 (1 << 3)


struct time_registers_t
{
	uint32_t CS;
	const uint32_t CLO;
	const uint32_t CHI;
	const uint32_t C0; // used by GPU
	uint32_t C1;
	const uint32_t C2; // used by GPU
	uint32_t C3;
};


static volatile struct time_registers_t *timer = (volatile struct time_registers_t *) (CPU_IO_BASE + 0x3000);


void st_initialize()
{
    // clear the interrupts by writing 1 to the corresponding bits
    timer->CS = BCM2835_COMPARER_1 | BCM2835_COMPARER_3;
}


uint64_t st_tick()
{
	sync_dataMemBarrier();

    uint32_t hv, lv;
	do {
		hv = timer->CHI;
		lv = timer->CLO;
	} while (hv != timer->CHI);

	sync_dataMemBarrier();

	return ((uint64_t)hv << 32 | lv);
}


/**
 * @brief Set the timer to interrupt after an amount of cycles from now.
 *
 * This function uses the C1 compare of the BCM2835 System Timer. An interrupt
 * will be generated when 'CLO' matches 'C1'.
 */
void st_update( uint32_t cycles )
{
    sync_dataMemBarrier();

    // clear the current interrupt by writing 1 to the corresponding bit
    timer->CS = BCM2835_COMPARER_1;
    // set the new value for the C1 comparer
    timer->C1 = timer->CLO + cycles;

    sync_dataMemBarrier();
}
