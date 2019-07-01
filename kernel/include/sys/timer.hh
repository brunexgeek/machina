/**
 * Provides access to BCM2835 system timer.
 *
 * For more information, consult the "BCM2835 ARM Peripherals" manual
 * on page 172.
 */

#ifndef MACHINA_TIMER_HH
#define MACHINA_TIMER_HH


#include <sys/types.h>
#include <sys/soc.h>


namespace machina {


struct TimerRegisters
{
	volatile uint32_t status;
	const volatile uint32_t counterLow;
	const volatile uint32_t counterHigh;
	volatile uint32_t compare0;
	volatile uint32_t compare1;
	volatile uint32_t compare2;
	volatile uint32_t compare3;
};


uint64_t timer_tick();
void timer_wait( uint64_t micro );

}

#endif // MACHINA_TIMER_HH