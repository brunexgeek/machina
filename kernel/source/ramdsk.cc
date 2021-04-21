#include <sys/heap.h>
#include <sys/device.hh>
#include <sys/errors.h>
#include <mc/stdio.h>

static device_driver_t def_driver;

static const uint32_t DEV_PRODUCT_ID = 0xFFFF;
static const uint32_t DEV_VENDOR_ID = 0xFFFF;
static const char *DEV_PRODUCT = "ramdisk";
static const char *DEV_VENDOR = "machina";

struct internal_t
{
    void *ptr;
    size_t size;
    int sector_size;
};

//
// Device API
//

static int drvapi_read( device_t *dev, size_t sector, void *buffer, size_t count )
{
    (void) dev;
    (void) sector;
    (void) buffer;
    (void) count;
    return EOK;
}

static int drvapi_write( device_t *dev, size_t sector, void *buffer, size_t count )
{
    (void) dev;
    (void) sector;
    (void) buffer;
    (void) count;
    return EOK;
}

static int drvapi_sync( device_t *dev )
{
    (void) dev;
    return EOK;
}

static int drvapi_ioctl( device_t *dev, int32_t cmd, void *buffer )
{
    (void) dev;
    (void) cmd;
    (void) buffer;
    return EOK;
}

static int drvapi_status( device_t *dev )
{
    (void) dev;
    return EOK;
}

//
// Driver
//

static int driver_attach(device_driver_t *drv, device_t *dev)
{
    (void) drv;
    (void) dev;
    dev->driver = drv;
	return EOK;
}

static int driver_remove(device_t *dev)
{
    (void) dev;
	return ENOIMP;
}

static int driver_suspend(device_t *dev)
{
    (void) dev;
	return ENOIMP;
}

static int driver_resume(device_t *dev)
{
    (void) dev;
	return ENOIMP;
}

int kramdsk_create_driver( system_bus_t *bus, device_driver_t **drv )
{
    if (drv == nullptr) return EARGUMENT;

    def_driver.bus = (bus == nullptr) ? kdev_local_bus() : bus;
    def_driver.name = DEV_PRODUCT;
    def_driver.attach = driver_attach;
    def_driver.remove = driver_remove;
    def_driver.suspend = driver_suspend;
    def_driver.resume = driver_resume;
    def_driver.internals = nullptr;
    def_driver.next = nullptr;
    def_driver.dev_api.storage.read = drvapi_read;
    def_driver.dev_api.storage.write = drvapi_write;
    def_driver.dev_api.storage.ioctl = drvapi_ioctl;
    def_driver.dev_api.storage.sync = drvapi_sync;
    def_driver.dev_api.storage.status = drvapi_status;
    *drv = &def_driver;

    return EOK;
}

//
// Device
//

int kramdsk_create_device( system_bus_t *bus, size_t size, device_t **dev )
{
    static const int sector_size = 512;
    static int devid = 0;
    if (dev == nullptr) return EARGUMENT;
    if (devid > 99) return ETOOLONG;

    size = (size + sector_size - 1) & (~(sector_size - 1));

    *dev = (device_t*) heap_allocate(
        sizeof(device_t) +      // device structure
        sizeof(internal_t) +    // internal information
        size +                  // disk space
        8);                     // device name ('ramfsXX')
    if (*dev == nullptr) return EMEMORY;

    bus = (bus == nullptr) ? kdev_local_bus() : bus;
    (*dev)->bus = bus;
    (*dev)->driver = nullptr;
    (*dev)->id = 0;
    (*dev)->id_product = 0;
    (*dev)->id_vendor = 0;
    (*dev)->iobase = nullptr;
    (*dev)->product = DEV_PRODUCT;
    (*dev)->vendor = DEV_VENDOR;
    (*dev)->next = nullptr;
    (*dev)->name = (char*)(*dev + 1);
    snprintf((char*)(*dev)->name, 7, "ramfs%d", devid++);
    (*dev)->internals = (void*) ((*dev)->name + 8);

    internal_t *tmp = (internal_t*) (*dev)->internals;
    tmp->ptr = tmp + 1;
    tmp->size = size;
    tmp->sector_size = sector_size;

    return bus->attach(bus, *dev);
}

int kramdsk_initialize( system_bus_t *bus, device_driver_t **drv )
{
    if (drv == nullptr) return EARGUMENT;

	int result = kramdsk_create_driver(bus, drv);
	if (result) return result;
	return kdev_register_driver(*drv = &def_driver);
}