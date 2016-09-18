#include <sys/Display.hh>
#include <sys/Mailbox.hh>
#include <sys/Screen.hh>
#include <sys/sync.h>
#include <sys/soc.h>
#include <mc/stdlib.h>
#include <mc/string.h>
#include <mc/memory.h>
#ifndef __arm__
#include <iostream>
#include <cstdlib>
#endif

namespace machina {


struct FrameBufferInfo
{
	uint32_t width;          // width of the physical display
	uint32_t height;         // height of the physical display
	uint32_t virtualWidth;   // width of the virtual framebuffer
	uint32_t virtualHeight;  // height of the virtual framebuffer
	uint32_t pitch;          // pitch (bytes per line, filled by GPU)
	uint32_t depth;          // depth (bits per pixel)
	uint32_t offsetX;        // X offset of the virtual framebuffer
	uint32_t offsetY;        // Y offset of the virtual framebuffer
	uint32_t bufferPtr;      // framebuffer address (filled by GPU)
	uint32_t bufferSize;     // framebuffer size (in bytes, filled by GPU)
};

/*
 * We use a global variable because we need a 16-bytes aligned storage.
 *
 * PS: 'aligned' attribute should/could not work in local variables.
 */
static FrameBufferInfo req __attribute((aligned (16)));


//static Display *instance;
static Display instance;


Display::Display (
	uint32_t width,
	uint32_t height,
	uint32_t depth ) : width(width), height(height), depth(DISPLAY_DEPTH),
		pitch(width * (depth / 8))
{

#if __arm__
	req.width = width;
	req.height = height;
	req.virtualWidth = width;
	req.virtualHeight = height;
	req.pitch = 0;
	req.depth = depth;
	req.offsetX = 0;
	req.offsetY = 0;
	req.bufferPtr = 0;
	req.bufferSize = 0;
	// https://github.com/raspberrypi/firmware/wiki/Mailbox-framebuffer-interface
	Mailbox::send(MAILBOX_CHANNEL_DISPLAY, GPU_MEMORY_BASE + (uint32_t) &req);

	buffer = (Color*) ( req.bufferPtr & 0x3FFFFFFF );
	bufferSize = req.bufferSize;
	pitch = req.pitch;
#else
	bufferSize = width * height * (DISPLAY_DEPTH / 8);
	buffer = (Color*) calloc(1, info.bufferSize);
#endif
}


Display::~Display ()
{
	// nothing to do
}


Display &Display::getInstance()
{
	/*if (instance == nullptr)
		instance = new Display();
	return *instance;*/
	return instance;
}


void Display::drawSomething(
	uint32_t posX,
	uint32_t posY,
	Color color )
{
	size_t offset = posY * pitch + posX;
	for (size_t i = 0; i < 20; ++i)
		buffer[offset + i] = color;
}


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

	size_t srcOffBeg = ( sizeof(Color) * (x < 0) ? -x : 0 ) +
		( (y < 0) ? -y : 0 ) * screen.getPitch();
	size_t srcPitch  = min( screen.getPitch(), pitch ) - sizeof(Color) * ( (x < 0) ? -x : x );

	size_t dstOffBeg = sizeof(Color) * ( (srcOffBeg == 0) ? x : 0 ) +
		( (srcOffBeg == 0) ? y : 0 ) * pitch;

	uint8_t *src = (uint8_t*) screen.getBuffer();
	uint8_t *dst = (uint8_t*) buffer + dstOffBeg;
	size_t cy = min( height, screen.getHeight() ) - y;
	for (; cy > 0; --cy)
	{
		CopyMemory(dst, src, srcPitch);
		src += screen.getPitch();
		dst += pitch;
	}
}


} // machina