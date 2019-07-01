#include <sys/timer.hh>


uint64_t timer_tick()
{
    const volatile uint64_t *value = (uint64_t*) (CPU_TIMER_BASE + 0x04);
    return *value;
}

void timer_wait( uint64_t micro )
{
    const volatile uint64_t *value = (uint64_t*) (CPU_TIMER_BASE + 0x04);
    micro += *value;

    while (*value < micro);
}