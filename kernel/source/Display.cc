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
FrameBufferInfo req __attribute((aligned (16)));


enum AnsiEscapeState
{
	AES_NONE,
	AES_ESCAPE,
	AES_BRACKET
};


Color DISPLAY_PALETTE[] =
{
	DISPLAY_COLOR(0x0E, 0x14, 0x16),
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


//static Display *instance = NULL;


Display::Display (
	uint32_t width,
	uint32_t height )
{
	width = DISPLAY_WIDTH;
	height = DISPLAY_HEIGHT;

	memset(info.lineMask, 1, sizeof(info.lineMask));
#if __arm__
	req.width = width;
	req.height = height;
	req.virtualWidth = req.width;
	req.virtualHeight = req.height;
	req.pitch = 0;
	req.depth = DISPLAY_DEPTH;
	req.offsetX = 0;
	req.offsetY = 0;
	req.bufferPtr = 0;
	req.bufferSize = 0;
	// https://github.com/raspberrypi/firmware/wiki/Mailbox-framebuffer-interface
	Mailbox::send(MAILBOX_CHANNEL_DISPLAY, GPU_MEMORY_BASE + (uint32_t) &req);

	info.buffer = (Color*) ( req.bufferPtr & 0x3FFFFFFF );
	info.bufferSize = req.bufferSize;
#else
	info.bufferSize = width * height * (DISPLAY_DEPTH / 8);
	info.buffer = (Color*) calloc(1, info.bufferSize);
#endif
	info.pitch      = width * (DISPLAY_DEPTH / 8);
	info.width      = width;
	info.height     = height;
	info.foreground = 9;
	info.background = 0;
	info.font = &Font::getMonospaceFont();
	info.scroll = false;

	info.columns   = width / info.font->getGlyphWidth();
	info.rows      = height / info.font->getGlyphHeight();
	info.textSize = info.columns * info.rows;
	memset(info.text, ' ', info.textSize);
	info.setOffset(0);

	memset(info.attribute, 0, sizeof(info.attribute));

	print("Video memory mapped to ");
	printHex( (uint32_t) info.bufferSize );
	print("\n");
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
				info.setOffset( info.getOffset() + info.columns );
				info.setOffset( info.getOffset() / info.columns * info.columns );
				memset(info.text + info.getOffset(), ' ', info.columns);
				break;

			case '\r':
				info.setOffset( info.getOffset() / info.columns * info.columns );
				break;

			default:
				info.text[ info.getOffset() ] = symbol;
				info.attribute[ info.getOffset() ] = (uint8_t) (info.foreground | (info.background << 4));
				info.setOffset( info.getOffset() + 1 );
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
}


void Display::clearLine()
{
	/*size_t offset = (info.textOffset / info.columns) * info.columns;
	memset(info.text + offset, ' ', info.columns);*/
}


void Display::refresh()
{
	uint32_t glyphW = info.font->getGlyphWidth();
	uint32_t glyphH = info.font->getGlyphHeight();

	size_t *ptr = (size_t*) info.buffer;
	size_t color = 	DISPLAY_PALETTE[0] << 16 | DISPLAY_PALETTE[0];
	#ifdef ARM_64
	color |= (color << 32);
	#endif
	for (size_t i = 0; i < info.bufferSize / sizeof(size_t); ++i)
		ptr[i] = color;

	size_t start = 0;
	if (info.scroll)
	{
		start = info.getOffset() / info.columns * info.columns;
		if (start == info.textSize - info.columns)
			start = 0;
		else
			start += info.columns;
	}

	uint32_t totalX = info.columns * glyphW;
	uint32_t totalY = info.rows * glyphH;

	for (uint32_t y = 0; y < totalY; y += glyphH)
	{
		for (uint32_t x = 0; x < totalX; x += glyphW)
		{
			size_t index = start % info.textSize;
			++start;

			draw(info.text[index], x, y,
				DISPLAY_PALETTE[ info.attribute[index] & 0x0F ],
				DISPLAY_PALETTE[ info.attribute[index] >> 4 ] );
		}
	}

}


void Display::draw(
	char symbol,
	uint32_t posX,
	uint32_t posY,
	Color foreground,
	Color background )
{
	uint32_t glyphW = info.font->getGlyphWidth();
	uint32_t glyphH = info.font->getGlyphHeight();

	const uint16_t *glyph = info.font->getGlyph(symbol);

	for (uint32_t y = 0; y < glyphH; ++y)
	{
		uint32_t offsetY = y + posY;
		for (uint32_t x = 0, bit = 1 << 15; x < glyphW; ++x, bit >>= 1)
		{
			Color current = ( glyph[y] & bit ) ? foreground : background;
			info.buffer[ offsetY * info.width + x + posX ] = current;
		}
	}
}


} // machina