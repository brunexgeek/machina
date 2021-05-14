#include <sys/heap.h>
#include <sys/device.hh>
#include <sys/errors.h>
#include <mc/stdio.h>

static device_driver_t def_driver;
static int devid = 0;

static const uint32_t DEV_PRODUCT_ID = 0xFFFF;
static const uint32_t DEV_VENDOR_ID = 0xFFFF;
static const char *DEV_PRODUCT = "ramdisk";
static const char *DEV_VENDOR = "machina";
static const int SECTOR_SIZE = 512;

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

static int drvapi_size( device_t *dev, size_t *size )
{
    *size = ((internal_t*) dev->internals)->size;
    return EOK;
}

//
// Driver
//

static int driver_attach(device_driver_t *drv, device_t *dev)
{
    if (!drv || !dev)
        return EARGUMENT;
    if (dev->id_product != DEV_PRODUCT_ID || dev->id_vendor != DEV_VENDOR_ID)
        return EINVALID;
    dev->driver = &def_driver;
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
    def_driver.dev_type = DEV_TYPE_STORAGE;
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
    def_driver.dev_api.storage.size = drvapi_size;
    *drv = &def_driver;

    return EOK;
}

//
// Device
//

int kramdsk_create_device( system_bus_t *bus, size_t size, device_t **dev )
{
    static int devid = 0;
    if (dev == nullptr) return EARGUMENT;
    if (devid > 99) return ETOOLONG;

    size = (size + SECTOR_SIZE - 1) & (~(SECTOR_SIZE - 1));
    void *data = heap_allocate(size);
    if (data == nullptr) return EMEMORY;

    int result = kdev_create_device(DEV_TYPE_STORAGE, DEV_VENDOR_ID,
        DEV_PRODUCT_ID, "ramdsk", sizeof(internal_t), dev);
    if (result)
    {
        heap_free(data);
        return result;
    }

    if (bus != nullptr)
        (*dev)->bus = bus;
    else
        bus = (*dev)->bus;
    (*dev)->product = DEV_PRODUCT;
    (*dev)->vendor = DEV_VENDOR;

    internal_t *tmp = (internal_t*) (*dev)->internals;
    tmp->ptr = data;
    tmp->size = size;
    tmp->sector_size = SECTOR_SIZE;

    return bus->attach(bus, *dev);
}

int kramdsk_create_device( system_bus_t *bus, void *ptr, size_t size, device_t **dev )
{
    if (ptr == nullptr || (size % SECTOR_SIZE) != 0) return EMEMORY;
    if (dev == nullptr) return EARGUMENT;
    if (devid > 99) return ETOOLONG;

    int result = kdev_create_device(DEV_TYPE_STORAGE, DEV_VENDOR_ID,
        DEV_PRODUCT_ID, "ramfs", sizeof(internal_t), dev);
    if (result) return result;

    if (bus != nullptr)
        (*dev)->bus = bus;
    else
        bus = (*dev)->bus;
    (*dev)->product = DEV_PRODUCT;
    (*dev)->vendor = DEV_VENDOR;

    internal_t *tmp = (internal_t*) (*dev)->internals;
    tmp->ptr = ptr;
    tmp->size = size;
    tmp->sector_size = SECTOR_SIZE;

    return bus->attach(bus, *dev);
}

int kramdsk_initialize( system_bus_t *bus, device_driver_t **drv )
{
    if (drv == nullptr) return EARGUMENT;

	int result = kramdsk_create_driver(bus, drv);
	if (result) return result;
	return kdev_register_driver(*drv = &def_driver);
}