#ifndef MACHINA_SYSIO_HH
#define MACHINA_SYSIO_HH


#include <sys/types.h>


static inline uint32_t GET32(
	const uint32_t *address )
{
	return *( (uint32_t volatile *) address );
}


static inline uint32_t GET32(
	const uint32_t address )
{
	return *( (uint32_t volatile *) address );
}


static inline void PUT32(
	uint32_t *address,
	uint32_t value )
{
	*( (uint32_t volatile *) address ) = value;
}


static inline void PUT32(
	uint32_t address,
	uint32_t value )
{
	*( (uint32_t volatile *) address ) = value;
}


#endif // MACHINA_SYSIO_HH