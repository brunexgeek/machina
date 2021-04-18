#ifndef MACHINA_DISPLAY_HH
#define MACHINA_DISPLAY_HH

#include <sys/types.h>
#include <sys/device.hh>

int kvid_initialize( system_bus_t *bus, device_t **dev, device_driver_t **drv );

#endif // MACHINA_DISPLAY_HH