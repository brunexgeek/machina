/**
 * Provides access to BCM2835 system timer.
 *
 * For more information, consult the "BCM2835 ARM Peripherals" manual
 * on page 172.
 */

#ifndef MACHINA_TIMER_HH
#define MACHINA_TIMER_HH


#include <sys/types.h>
#include <sys/bcm2837.h>


struct timer_t
{
	uint64_t (*now)();
};


void timer_initialize();
void timer_terminate();
void timer_register( struct timer_t &timer );
uint64_t timer_tick();
void timer_wait( uint64_t us );


#endif // MACHINA_TIMER_HH