#include <sys/timer.hh>
#include <sys/sync.h>


#define BCM2835_COMPARER_1 (1 << 1)
#define BCM2835_COMPARER_3 (1 << 3)


struct time_registers_t
{
	uint32_t load;
	const uint32_t value;
	uint32_t control;
	uint32_t clear;
	const uint32_t raw;
	const uint32_t marked;
	uint32_t reload;
    uint32_t prediv;
    uint32_t freerun;
};


static volatile struct time_registers_t *timer = (volatile struct time_registers_t *) (CPU_IO_BASE + 0xB400);


void at_initialize()
{
    timer->control = timer->control | 1 << 9;
}


uint64_t at_tick()
{
	sync_dataMemBarrier();

    uint32_t value = timer->freerun;

	sync_dataMemBarrier();

	return value;
}


/**
 * @brief Set the timer to interrupt 'cyles' from now.
 *
 * This function uses the C1 compare of the BCM2835 System Timer. An interrupt
 * will be generated when 'CLO' matches 'C1'.
 */
void at_update( uint32_t cycles )
{

}
