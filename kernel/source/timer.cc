#include <sys/timer.hh>
#include <sys/sync.h>


void st_initialize();
uint64_t st_tick();
void st_update( uint32_t cycles );


void timer_initialize()
{
	st_initialize();
	st_update(100000);
}


uint64_t timer_tick()
{
	return st_tick();
}


void timer_wait( uint64_t micro )
{
    micro += st_tick();
    while (st_tick() < micro);
}


