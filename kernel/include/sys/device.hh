#ifndef MACHINA_DEVICE_HH
#define MACHINA_DEVICE_HH

#include <sys/types.h>

struct device_t;
struct device_driver_t;

struct system_bus_t
{
    char *name;
    /**
     * Attach a new device to the bus.
     *
     * If any compatible driver is found, calls the driver's @c attach function
     * with the device. Returns an error otherwise.
     */
    int (*attach)(system_bus_t *bus, device_t *dev);
    /**
     * Called when a device is removed from the bus.
     */
    int (*remove)(device_t *dev);
    /**
     * Put the device in sleep mode (e.g. low power mode).
     */
    int (*suspend)(device_t *dev);
    /**
     * Bring the device back from sleep mode.
     */
    int (*resume)(device_t *dev);
    /**
     * Pointer to the next item in the linked list or nullptr otherwise.
     */
    system_bus_t *next;
    device_t *devices;
};

enum device_type
{
    DEV_TYPE_VIDEO,
    DEV_TYPE_STORAGE
};

enum pixel_format
{
    PIXEL_FORMAT_RGB565 // 16-bits per pixel
};

struct video_api
{
    int (*clear)( device_t *dev, uint32_t color );
    int (*draw)( device_t *dev, void *pixels, int width, int height, int x, int y, int pitch );
};

struct storage_api {};

union device_api
{
    video_api video;
    storage_api storage;
};

struct device_driver_t
{
    const char *name;
    struct system_bus_t *bus;
    device_type dev_type;
    /**
     * Object holding the function pointers for the device API. The API to be used
     * is defined by the field @c clazz.
     */
    device_api dev_api;
    /**
     * Called to query the existence of a specific device, whether this driver can work with it, and bind the driver to a specific device.
     */
    int (*attach)(device_driver_t *drv, device_t *dev);
    /**
     * Called when a device removed from this bus.
     */
    int (*remove)(device_t *dev);
    /**
     * Put the device in sleep mode (e.g. low power mode).
     */
    int (*suspend)(device_t *dev);
    /**
     * Bring the device back from sleep mode.
     */
    int (*resume)(device_t *dev);
    /**
     * Driver's private data.
     */
    void *internals;
    device_driver_t *next;
};

struct device_t
{
    const char *name;
    const char *vendor;
    uint32_t id;               // internal device ID (relative to the bus)
    uint32_t id_product;
    uint32_t id_vendor;
    device_driver_t *driver;
    system_bus_t *bus;
    void *iobase;
    /**
     * Internal data managed by device drivers
     */
    void *internals;
    device_t *next;
};

/**
 * Initialize the device subsystem.
 */
int kdev_initialize();

/**
 * Enumerate all attached devices.
 */
int kdev_enumerate();

/**
 * Enumerate all devices attached to the bus.
 */
int kdev_enumerate_bus( system_bus_t *bus );

/**
 * Register a new device driver.
 */
int kdev_register_driver( device_driver_t *drv );

#endif // MACHINA_DEVICE_HH