#include <sys/timer.hh>
#include <sys/sync.h>


void lt_initialize()
{

}


uint64_t lt_tick()
{
	sync_dataMemBarrier();

    uint32_t value = *((uint32_t*)0x4000001C);

	sync_dataMemBarrier();

	return value;
}


/**
 * @brief Set the timer to interrupt 'cyles' from now.
 *
 * This function uses the C1 compare of the BCM2835 System Timer. An interrupt
 * will be generated when 'CLO' matches 'C1'.
 */
void lt_update( uint32_t cycles )
{

}
