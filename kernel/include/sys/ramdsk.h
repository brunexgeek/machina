#ifndef MACHINA_RAMDSK_H
#define MACHINA_RAMDSK_H

int kramdsk_initialize( device_driver_t **drv );

int kramdsk_create_device( system_bus_t *bus, size_t size, device_t **dev );

#endif // MACHINA_RAMDSK_H