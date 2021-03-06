#include <sys/errors.h>
#include <sys/uart.h>
#include <sys/device.hh>
#include <sys/heap.h>
#include <mc/stdio.h>
#include <mc/string.h>

static system_bus_t *bus_list = nullptr;
static device_driver_t *driver_list = nullptr;

static system_bus_t *kdev_create_bus( const char *name )
{
    system_bus_t *bus = (system_bus_t*) heap_allocate( sizeof(system_bus_t) + strlen(name) + 1 );
    if (bus == nullptr) return nullptr;
    bus->name = (char*) bus + sizeof(system_bus_t);
    strcpy(bus->name, name);
    return bus;
}

/*
 * Local bus
 */

static int local_bus_match(device_t *dev, device_driver_t *driver)
{
    return EOK;
}

static int local_bus_attach(system_bus_t *bus, device_t *dev)
{
    int result = EINVALID;

    // attach the driver
    if (dev->driver == nullptr)
    {
        // try to find a compatible driver
        device_driver_t *drv = driver_list;
        for (;result != EOK && drv; drv = drv->next)
            result = drv->attach(drv, dev);
        if (result != EOK)
            return result;
    }

    // append the device
    dev->next = bus->devices;
    bus->devices = dev;

    return EOK;
}

static int local_bus_remove(device_t *dev)
{
    return dev->driver->remove(dev);
}

static int local_bus_suspend(device_t *dev)
{
    return dev->driver->suspend(dev);
}

static int local_bus_resume(device_t *dev)
{
    return dev->driver->resume(dev);
}

static system_bus_t *kdev_create_local_bus()
{
    system_bus_t *bus = kdev_create_bus("local");
    if (bus == nullptr) return nullptr;
    bus->attach = local_bus_attach;
    bus->remove = local_bus_remove;
    bus->suspend = local_bus_suspend;
    bus->resume = local_bus_resume;
    bus->next = bus_list;
    bus->devices = nullptr;
    puts("Created bus \"local\"\n");
    return bus_list = bus;
}

//
// Device API
//

int kvid_initialize( system_bus_t *bus, device_t **dev, device_driver_t **drv ); // display.hh

int kdev_initialize()
{
    int result = EOK;
    system_bus_t *bus = kdev_create_local_bus();
    if (bus)
    {
        device_t *dev;
        device_driver_t *drv;
        result = kvid_initialize(bus, &dev, &drv);
    }
    return result;
}

int kdev_enumerate()
{
    system_bus_t *bus = bus_list;
    while (bus)
    {
        kdev_enumerate_bus(bus);
        bus = bus->next;
    }
    return EOK;
}

int kdev_enumerate_bus( system_bus_t *bus )
{
    device_t *dev = bus->devices;
    while (dev)
    {
        uart_print("[%s.%d] [%04X:%04X] %s (%s)\n",
            bus->name,
            dev->id,
            dev->id_product,
            dev->id_vendor,
            dev->name,
            dev->driver->name);
        dev = dev->next;
    }
    return EOK;
}

int kdev_register_driver( device_driver_t *drv )
{
    if (drv == nullptr) return EARGUMENT;

    // check whether the driver is already registered
    device_driver_t *tmp = driver_list;
    while (tmp)
    {
        if (tmp == drv || strcmp(tmp->name, drv->name) == 0)
            return EEXIST;
        tmp = tmp->next;
    }
    // include the driver
    drv->next = driver_list;
    driver_list = drv;

    return EOK;
}