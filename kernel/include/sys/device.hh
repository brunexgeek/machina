#ifndef MACHINA_DEVICE_HH
#define MACHINA_DEVICE_HH

#include <sys/cpp.h>
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
    /**
     * Device list.
     */
    linked_list_t<device_t> devices;
    /**
     * Device counter used to set device ids.
     */
    int counter;
};

enum device_type
{
    DEV_TYPE_UNKNOWN,
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

struct storage_api
{
    /**
     * Read data to the device.
     *
     * @param dev Device pointer.
     * @param sector sector Number of the sector to read from. If @c count is greater than
     *     1 the following sectors will be read.
     * @param buffer Pointer to the buffer. Must have (@c count * sector
     *     size) bytes.
     * @param count Number of sector to read.
     */
    int (*read)( device_t *dev, size_t sector, void *buffer, size_t count );
    /**
     * Write data to the device.
     *
     * @param dev Device pointer.
     * @param sector sector Number of the sector to write from. If @c count is greater than
     *     1 the following sectors will be written.
     * @param buffer Pointer to the data to be written. Must have (@c count * sector
     *     size) bytes.
     * @param count Number of sector to write.
     */
    int (*write)( device_t *dev, size_t sector, void *buffer, size_t count );
    /**
     * Makes sure that the device has finished pending write process.
     *
     * @param dev Device pointer.
     */
    int (*sync)( device_t *dev );
    /**
     * Access device-specific functions.
     *
     * @param dev Device pointer.
     * @param cmd Command identifier.
     * @param buffer Input/output buffer
     */
    int (*ioctl)( device_t *dev, int32_t cmd, void *buffer );
    int (*status)( device_t *dev );
};

union device_api
{
    video_api video;
    storage_api storage;
};

struct device_driver_t
{
    /**
     * Unique name used to publish the driver in '/drv' directory.
     */
    const char *name;
    struct system_bus_t *bus;
    /**
     * Type of device that this driver associates with.
     *
     * This acts somewhat like USB device class. Can not be @c DEV_TYPE_UNKNOWN.
     */
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
    /**
     * Pointer to the next driver in the linked list.
     */
    device_driver_t *next;
};

struct device_t
{
    /**
     * Unique name used to publish the device in '/dev' directory.
     *
     * If the device has a parent, its name is composed by the parent name
     * followed by a dot (.) and the device id (e.g. 'sd1.1' for a device
     * representing the second partition of the storage device 'sd1').
     */
    const char *name;
    /**
     * Unique product identifier.
     *
     * Usually this is the USB or PCI product id for the device. If the
     * device is from another bus (e.g. local), this value can be anything
     * which don't conflict with standardized values.
     *
     * A bus use this to match the correct driver for the device.
     */
    uint32_t id_product;
    /**
     * Unique vendor identifier.
     *
     * Usually this is the USB or PCI vendor id for the device. If the
     * device is from another bus (e.g. local), this value can be anything
     * which don't conflict with standardized values.
     *
     * A bus use this to match the correct driver for the device.
     */
    uint32_t id_vendor;
    /**
     * Product name matching the product id.
     */
    const char *product;
    /**
     * Vendor name matching the vendor id.
     */
    const char *vendor;
    /**
     * Device type.
     *
     * This acts somewhat like USB device class. Until the device class is unknown,
     * this field should be @c DEV_TYPE_UNKNOWN.
     */
    uint32_t type : 4;
    /**
     * Device identifier used in the name.
     *
     * If the device has no parent, this id is generated by an internal counter of
     * devices per type. Otherwise, this id is given by the parent device.
     */
    uint32_t name_id : 8;
    /**
     * Device identifier relative to the bus.
     */
    uint32_t dev_id : 8;
    uint32_t reserved : 12;
    /**
     * Pointer to the driver selected to manage this device.
     */
    device_driver_t *driver;
    /**
     * Pointer to the bus.
     */
    system_bus_t *bus;
    /**
     * Pointer to the IO base.
     */
    void *iobase;
    /**
     * Internal data managed by device drivers.
     */
    void *internals;
    /**
     * Pointer to parent device. This is usefull for devices like disks
     * where partitions can be defined as sub-devices.
     */
    device_t *parent;
    /**
     * Pointer to the next device in the linked list.
     */
    device_t *next;
};

/**
 * Initialize the device subsystem.
 */
int kdev_initialize();

/**
 * Enumerate all drivers.
 */
int kdev_enumerate_driver();

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

/**
 * Returns the local bus pointer.
 */
system_bus_t *kdev_local_bus();

/**
 * Create an initialized device object.
 *
 * This function can be used to create real and virtual devices.
 *
 * Virtual devices enables virtual drivers to use software components as if it were real devices.
 * This can be usefull if you want to redirect device interactions to other software
 * (e.g. video device/driver that redirects the output to another computer via network).
 */
int kdev_create_device( device_type type, uint32_t vendor, uint32_t product,
    const char *name, size_t internal_size, device_t **dev );

/**
 * Find a device by name.
 *
 * Since the device name do not give any clue about the bus,
 * this function will search for the device in every bus available.
 */
int kdev_find( const char *name, device_t **dev );

/**
 * Find a bus by name.
 */
int kdev_find( const char *name, system_bus_t **bus );

#endif // MACHINA_DEVICE_HH