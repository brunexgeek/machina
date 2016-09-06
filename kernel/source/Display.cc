#include <sys/Display.hh>
#include <sys/Mailbox.hh>
#include <sys/Screen.hh>
#include <sys/sync.h>
#include <sys/soc.h>
#include <mc/stdlib.h>
#include <mc/string.h>
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


static Display instance;


Display::Display (
	uint32_t width,
	uint32_t height,
	uint32_t depth ) : width(width), height(height), depth(DISPLAY_DEPTH)
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
#else
	bufferSize = width * height * (DISPLAY_DEPTH / 8);
	buffer = (Color*) calloc(1, info.bufferSize);
#endif
	//memset(buffer, 0xff, bufferSize);
}


Display::~Display ()
{
	// nothing to do
}


Display &Display::getInstance()
{
	return instance;
}


const char* Display::getName() const
{
	return "Display";
}


const char* Display::getFileName() const
{
	return "display";
}


void Display::clearLine()
{
	/*size_t offset = (info.textOffset / info.columns) * info.columns;
	memset(info.text + offset, ' ', info.columns);*/
}



void Display::draw(
	const char *text,
	uint32_t posX,
	uint32_t posY,
	const Font &font,
	Color foreground,
	Color background )
{
	for (; *text != 0; ++text);
		draw(*text, posX, posY, font, foreground, background);
}


void Display::draw(
	char symbol,
	uint32_t posX,
	uint32_t posY,
	const Font &font,
	Color foreground,
	Color background )
{
	uint32_t glyphW = font.getGlyphWidth();
	uint32_t glyphH = font.getGlyphHeight();

	const uint16_t *glyph = font.getGlyph(symbol);

	for (uint32_t y = 0; y < glyphH; ++y)
	{
		uint32_t offsetY = y + posY;
		for (uint32_t x = 0, bit = 1 << 15; x < glyphW; ++x, bit >>= 1)
		{
			Color current = ( glyph[y] & bit ) ? foreground : background;
			buffer[ offsetY * width + x + posX ] = current;
		}
	}
}


void Display::draw(
	const TextScreen &screen )
{
	size_t size = min( screen.info.bufferSize, bufferSize );
	memcpy(buffer, screen.info.buffer, size);
}


} // machina