#ifndef MACHINA_SYSIO_HH
#define MACHINA_SYSIO_HH


#include <sys/types.h>

#define GET32(addr) (*( (uint32_t volatile *) addr ))
#define PUT32(addr, value) do { *( (volatile uint32_t *) addr ) = (uint32_t) value; } while(false)

#endif // MACHINA_SYSIO_HH