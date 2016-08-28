#include <sys/Display.hh>
#include <sys/Mailbox.hh>
#include <sys/sync.h>
#include <sys/soc.h>
#include <sys/stdlib.hh>
#ifndef __arm__
#include <iostream>
#include <cstdlib>
#endif

namespace machina {


// This struct is shared between GPU and ARM side
// Must be 16 byte aligned in memory
struct Bcm2835FrameBufferInfo
{
	uint32_t Width;		// Physical width of display in pixel
	uint32_t Height;		// Physical height of display in pixel
	uint32_t VirtWidth;		// always as physical width so far
	uint32_t VirtHeight;		// always as physical height so far
	uint32_t Pitch;		// Should be init with 0
	uint32_t Depth;		// Number of bits per pixel
	uint32_t OffsetX;		// Normally zero
	uint32_t OffsetY;		// Normally zero
	uint32_t BufferPtr;		// Address of frame buffer (init with 0, set by GPU)
	uint32_t BufferSize;		// Size of frame buffer (init with 0, set by GPU)

	uint16_t Palette[0];		// with Depth <= 8 only (256 entries)
#define PALETTE_ENTRIES		256
};

Bcm2835FrameBufferInfo req __attribute((aligned (16)));

enum AnsiEscapeState
{
	AES_NONE,
	AES_ESCAPE,
	AES_BRACKET
};


Color DISPLAY_PALETTE[] =
{
	DISPLAY_COLOR(0x2E, 0x34, 0x36),
	DISPLAY_COLOR(0xCC, 0x00, 0x00),
	DISPLAY_COLOR(0x4E, 0x9A, 0x06),
	DISPLAY_COLOR(0xC4, 0xA0, 0x00),
	DISPLAY_COLOR(0x34, 0x65, 0xA4),
	DISPLAY_COLOR(0x75, 0x50, 0x7B),
	DISPLAY_COLOR(0x06, 0x20, 0x9A),
	DISPLAY_COLOR(0xD3, 0xD7, 0xCF),
	DISPLAY_COLOR(0x00, 0x00, 0x00), // not used
	DISPLAY_COLOR(0xD3, 0xD7, 0xCF), // default color
	DISPLAY_COLOR(0x55, 0x57, 0x53),
	DISPLAY_COLOR(0xEF, 0x29, 0x29),
	DISPLAY_COLOR(0x8A, 0xE2, 0x34),
	DISPLAY_COLOR(0xFC, 0xE9, 0x4F),
	DISPLAY_COLOR(0x72, 0x9F, 0xCF),
	DISPLAY_COLOR(0xAD, 0x7F, 0xA8),
	DISPLAY_COLOR(0x34, 0xE2, 0xE2),
	DISPLAY_COLOR(0xEE, 0xEE, 0xEC),
	DISPLAY_COLOR(0x00, 0x00, 0x00), // not used
	DISPLAY_COLOR(0xEE, 0xEE, 0xEC), // default color
};


//static Display instance(DISPLAY_WIDTH, DISPLAY_HEIGHT);


Display::Display (
	uint32_t width,
	uint32_t height )
{
	width = DISPLAY_WIDTH;
	height = DISPLAY_HEIGHT;

	memset(info.lineMask, 1, sizeof(info.lineMask));
#if __arm__
	req.Width = width;
	req.Height = height;
	req.VirtWidth = req.Width;
	req.VirtHeight = req.Height;
	req.Pitch = 0;
	req.Depth = 16;
	req.OffsetX = 0;
	req.OffsetY = 0;
	req.BufferPtr = 0;
	req.BufferSize = 0;

	// https://github.com/raspberrypi/firmware/wiki/Mailbox-framebuffer-interface
	Mailbox::send(MAILBOX_CHANNEL_DISPLAY, GPU_MEM_BASE + (uint32_t) &req);

	info.buffer = (Color*) ( req.BufferPtr & 0x3FFFFFFF );
	info.bufferSize = req.BufferSize;
#else
	info.bufferSize = width * height * (DISPLAY_DEPTH / 8);
	info.buffer = (Color*) calloc(1, info.bufferSize);
#endif
	info.pitch      = width * (DISPLAY_DEPTH / 8);
	info.width      = width;
	info.height     = height;
	info.foreground = 9;
	info.background = 0;
	info.font = Font::getInstance();

	info.columns   = width / info.font.getGlyphWidth();
	info.rows      = height / info.font.getGlyphHeight();
	info.textSize = info.columns * info.rows;
	memset(info.text, ' ', info.textSize);
	info.textOffset = 0;

	memset(info.attribute, 0, sizeof(info.attribute));
	for (size_t i = 0; i < info.bufferSize / sizeof(Color); ++i)
		info.buffer[i] = DISPLAY_PALETTE[1];
}

Display::~Display ()
{
	// nothing to do
}


/*Display &Display::getInstance()
{
	return instance;
}*/


const char* Display::getName() const
{
	return "Display";
}


const char* Display::getFileName() const
{
	return "display";
}


void Display::print(
	const char *text,
	bool  )
{
	// acquire lock

	size_t length = strlen(text);

	for (size_t i = 0; i < length; ++i)
	{
		if (text[i] == '\t')
			print("    ");
		else
			print(text[i]);
	}

	// release lock
}


void Display::print(
	char symbol )
{
	static AnsiEscapeState state = AES_NONE;
	static size_t param = AES_NONE;

	if (state == AES_NONE)
	{
		switch (symbol)
		{
			case '\e':
				state = AES_ESCAPE;
				param = 0;
				break;

			case '\n':
				info.textOffset = (info.textOffset + info.columns) % info.textSize;
				// pass through

			case '\r':
				info.textOffset = (info.textOffset / info.columns) * info.columns;
				clearLine();
				break;

			default:
				info.text[ info.textOffset ] = symbol;
				info.attribute[ info.textOffset ] = (uint8_t) (info.foreground | (info.background << 4));
				info.textOffset = (info.textOffset + 1) % info.textSize;
				break;
		}
	}
	else
	if (state == AES_ESCAPE)
	{
		param = 0;

		switch (symbol)
		{
			case '[':
				state = AES_BRACKET;
				break;

			default:
				state = AES_NONE;
				break;
		}
	}
	else
	if (state == AES_BRACKET)
	{
		switch (symbol)
		{
			case 'm':
				state = AES_NONE;
				// pass through

			case ';':
				if (param == 0)
				{
					info.foreground = 9;
					info.background = 0;
				}
				if (param >= 30 && param <= 39)
				{
					info.foreground = param - 30;
				}
				else
				if (param >= 40 && param <= 49)
				{
					info.background = param - 40;
				}
				param = 0;
				break;

			default:
				if (symbol >= '0' && symbol <= '9')
				{
					if (param == 0)
						param = symbol - '0';
					else
					if (param <= 9)
						param = param * 10 + (symbol - '0');
					else
						state = AES_NONE;
				}
				else
					state = AES_NONE;
				break;
		}
	}

	// every time we reach the begin of a new line we need to
	// clean that line
	if ((info.textOffset % info.columns) == 0)
		memset(info.text + info.textOffset, ' ', info.columns);
}


void Display::clearLine()
{
	size_t offset = (info.textOffset / info.columns) * info.columns;
	memset(info.text, ' ', info.columns);
}


void Display::refresh()
{
	uint32_t glyphW = info.font.getGlyphWidth();
	uint32_t glyphH = info.font.getGlyphHeight();

	for (size_t i = 0; i < info.bufferSize / sizeof(Color); ++i)
		info.buffer[i] = DISPLAY_PALETTE[0];

	for (size_t i = 0; i < info.textSize; ++i)
	{
		if (info.text[i] == ' ') continue;

		uint32_t x = (i % info.columns) * glyphW;
		uint32_t y = (i / info.columns) * glyphH;
		draw(info.text[i], x, y, DISPLAY_PALETTE[ info.attribute[i] & 0x0F ],
			DISPLAY_PALETTE[ info.attribute[i] >> 4 ] );
	}
}


void Display::draw(
	char symbol,
	uint32_t posX,
	uint32_t posY,
	Color foreground,
	Color background )
{
	uint32_t glyphW = info.font.getGlyphWidth();
	uint32_t glyphH = info.font.getGlyphHeight();

	for (uint32_t y = 0; y < glyphH; ++y)
	{
		for (uint32_t x = 0; x < glyphW; ++x)
		{
			Color current = info.font.getBitUnsafe(symbol, x, y) ? foreground : background;
			//Color current = (x == y) ? 0xffff : 0x00;
			info.buffer[ (y + posY) * info.width + x + posX ] = current;
		}
	}
}



} // machina