#include <sys/errors.h>
#include <sys/display.hh>
#include <sys/mailbox.h>
#include <sys/Screen.hh>
#include <sys/sync.h>
#include <mc/stdlib.h>
#include <mc/string.h>
#include <sys/uart.h>
#include <sys/heap.h>
#ifndef __arm__
#include <iostream>
#include <cstdlib>
#endif

#define LOG_TITLE  "<display> "

struct __attribute__((__packed__, aligned(4))) vc4_fb_info
{
	uint32_t width;           // width of the physical display
	uint32_t height;          // height of the physical display
	uint32_t virtual_width;   // width of the virtual framebuffer
	uint32_t virtual_height;  // height of the virtual framebuffer
	uint32_t pitch;           // pitch (bytes per line, filled by GPU)
	uint32_t depth;           // depth (bits per pixel)
	uint32_t offset_x;        // X offset of the virtual framebuffer
	uint32_t offset_y;        // Y offset of the virtual framebuffer
	uint32_t buffer_ptr;      // framebuffer address (filled by GPU)
	uint32_t buffer_size;    // framebuffer size (in bytes, filled by GPU)
};

/*
 * We use a global variable because we need a 16-bytes aligned storage.
 *
 * PS: 'aligned' attribute should/could not work in local variables.
 */
//static __attribute((aligned (16))) vc4_fb_info req;

struct kvid_devinternals
{
	uint8_t *buffer;
	size_t   buffer_size;
	int32_t  width;
	int32_t  height;
	int32_t  depth;
	int32_t  pitch;
	int32_t  pixel_size; // bytes per pixel
};

static uint32_t DEV_PRODUCT_ID = 0xF7FF; // fake ID
static uint32_t DEV_VENDOR_ID = 0x0A5C; // Broadcom
static const char *DEV_NAME = "VideoCore IV";
static const char *DEV_VENDOR = "Broadcom";
static device_driver_t def_driver;
static video_api def_api;

// WARN: without the option '-fno-threadsafe-statics' this requires
// '___cxa_guard_acquire' and '___cxa_guard_release' functions
static int constexpr kdev_strlen( const char* str )
{
    return *str ? 1 + kdev_strlen(str + 1) : 0;
}

void kernel_panic( const char *path, int line );

static int kvid_api_clear( device_t *dev, uint32_t color )
{
	return ENOIMP;
}

static int kvid_api_draw( device_t *dev, void *pixels, int width, int height, int x, int y, int pitch )
{
	if (dev == nullptr || pixels == nullptr) return EARGUMENT;
	auto &internals = *((kvid_devinternals*) dev->internals);

	if (x < 0 || x + width >= (int32_t) internals.width ||
	    y < 0 || y + height >= (int32_t) internals.height)
		return ETOOLONG;

	int32_t offset = (y * internals.pitch + x) * internals.pixel_size;
	uint8_t *src = (uint8_t*) pixels;
	uint8_t *dst = (uint8_t*) internals.buffer + offset;
	for (; height > 0; --height)
	{
		memcpy(dst, src, width * internals.pixel_size);
		src += pitch;
		dst += internals.pitch;
	}
	return EOK;
}

static int kvid_drv_attach(device_driver_t *drv, device_t *dev)
{
	if (drv == nullptr || dev == nullptr)
		return EARGUMENT;
	if (dev->id_vendor != DEV_VENDOR_ID || dev->id_product != DEV_PRODUCT_ID)
		return EINVALID;

	static kvid_devinternals int_device;

	STACK_STRUCT_ALIGNED(req, sizeof(vc4_fb_info), 16, vc4_fb_info);

	int_device.width = 800;   // find out native resolution
	int_device.height = 600;  // find out native resolution
	int_device.depth = 16;
	int_device.pitch = 0;

	req.virtual_width = req.width = int_device.width;
	req.virtual_height = req.height = int_device.height;
	req.pitch = int_device.pitch;
	req.depth = int_device.depth;
	req.offset_x = 0;
	req.offset_y = 0;
	req.buffer_ptr = 0;
	req.buffer_size = 0;

	// TODO: this is deprecated! should use mailbox tags instead
	// https://github.com/raspberrypi/firmware/wiki/Mailbox-framebuffer-interface
	if (!mailbox_send(MB_CHANNEL_FB, (uint32_t) (uintptr_t) &req))
		kernel_panic(__FILE__, __LINE__);

	int_device.buffer = (uint8_t*) (uintptr_t) ( req.buffer_ptr & 0x3FFFFFFF );
	int_device.buffer_size = req.buffer_size;
	int_device.pitch = req.pitch;
	dev->internals = &int_device;
	dev->name = DEV_NAME;
	dev->vendor = DEV_VENDOR;
	dev->driver = &def_driver;
	dev->iobase = nullptr; // VC4 uses mailboxes

	uart_print("%s %dx%d at 0x%08x\n", DEV_NAME, req.width, req.height, int_device.buffer);

	return EOK;
}

static int kvid_drv_remove(device_t *dev)
{
	return ENOIMP;
}

static int kvid_drv_suspend(device_t *dev)
{
	return ENOIMP;
}

static int kvid_drv_resume(device_t *dev)
{
	return ENOIMP;
}

static int kvid_create_device( system_bus_t *bus )
{
	if (bus == nullptr) return EARGUMENT;

	static device_t def_device;

	def_device.name = DEV_NAME;
	def_device.vendor = DEV_VENDOR;
	def_device.id_vendor = DEV_VENDOR_ID;
	def_device.id_product = DEV_PRODUCT_ID;
	def_device.internals = nullptr;
	def_device.driver = nullptr;
	def_device.iobase = nullptr;

	return bus->attach(bus, &def_device);
}

int kvid_create_driver( system_bus_t *bus )
{
	if (bus == nullptr) return EARGUMENT;

	def_api.clear = kvid_api_clear;
	def_api.draw = kvid_api_draw;

	def_driver.name = "broadcom.vc4";
	def_driver.bus = bus;
	def_driver.dev_type = DEV_TYPE_VIDEO;
	def_driver.dev_api.video = def_api;
	def_driver.attach = kvid_drv_attach;
	def_driver.remove = kvid_drv_remove;
	def_driver.suspend = kvid_drv_suspend;
	def_driver.resume = kvid_drv_resume;
	def_driver.internals = nullptr;

	return EOK;
}

int kvid_initialize( system_bus_t *bus, device_t **dev, device_driver_t **drv )
{
	if (bus == nullptr || dev == nullptr || drv == nullptr) return EARGUMENT;

	int result = kvid_create_driver(bus);
	if (result) return result;
	result = kdev_register_driver(&def_driver);
	if (result) return result;
	result = kvid_create_device(bus);
	return result;
}

#if 0
void Display::draw(
	char symbol,
	uint32_t posX,
	uint32_t posY,
	const Font &font,
	Color foreground,
	Color /* background */ )
{
	uint32_t glyphW = font.getGlyphWidth();
	uint32_t glyphH = font.getGlyphHeight();

	const uint8_t *glyph = font.getGlyph(symbol);

	for (register uint32_t y = 0; y < glyphH; ++y)
	{
		uint32_t offset = (y + posY) * width + posX;
		uint32_t glyphIndex = y *glyphW;

		for (register uint32_t x = 0; x < glyphW; ++x)
		{
			if (glyph[glyphIndex + x] != 0)
				buffer[ offset+ x ] = foreground;
		}
	}
}


void Display::draw(
	const TextScreen &screen,
	int32_t x,
	int32_t y )
{
	if (screen.getDepth() != depth) return;

	if (x + (int32_t) screen.getWidth() < 0 || x >= (int32_t) width)
		return;
	if (y + (int32_t) screen.getHeight() < 0 || y >= (int32_t) width)
		return;

	size_t srcOffBeg = sizeof(Color) * ( (x < 0) ? -x : 0 ) +
		( (y < 0) ? -y : 0 ) * screen.getPitch();
	size_t srcPitch  = min( screen.getPitch(), pitch ) - sizeof(Color) * ( (x < 0) ? -x : x );

	size_t dstOffBeg = sizeof(Color) * ( (srcOffBeg == 0) ? x : 0 ) +
		( (srcOffBeg == 0) ? y : 0 ) * pitch;

	uint8_t *src = (uint8_t*) screen.getBuffer();
	uint8_t *dst = (uint8_t*) buffer + dstOffBeg;
	size_t cy = min( height, screen.getHeight() ) - y;
	for (; cy > 0; --cy)
	{
		memcpy(dst, src, srcPitch);
		src += screen.getPitch();
		dst += pitch;
	}
}
#endif
