#include <sys/Screen.hh>
#include <sys/pmm.hh>
#include <sys/system.h>
#include <mc/string.h>
#include <mc/stdio.h>
#include <mc/stdarg.h>
#include <mc/memory.h>
#include <sys/types.h>


namespace machina {


Color DISPLAY_PALETTE[] =
{
	RGB565_COLOR(0x212429), // black
	RGB565_COLOR(0xe94783), // red
	RGB565_COLOR(0x92bc18), // green
	RGB565_COLOR(0xf3a633), // brown
	RGB565_COLOR(0x5b94cd), // blue
	RGB565_COLOR(0x9f71fd), // magenta
	RGB565_COLOR(0x65cdde), // cyan
	RGB565_COLOR(0xd5d6d0), // gray
	0,
	RGB565_COLOR(0xd5d6d0), // default color (gray)

	RGB565_COLOR(0x484e50), // strong black
	RGB565_COLOR(0xe980a7), // strong red
	RGB565_COLOR(0xbfdb6d), // strong green
	RGB565_COLOR(0xf3db33), // strong brown
	RGB565_COLOR(0x8eb6ed), // strong blue
	RGB565_COLOR(0xbfa0fd), // strong magenta
	RGB565_COLOR(0x8ee9f8), // strong cyan
	RGB565_COLOR(0xf5f5f5), // strong gray
	0,
	RGB565_COLOR(0xf5f5f5), // default color (gray)
};


enum AnsiEscapeState
{
	AES_NONE,
	AES_ESCAPE,
	AES_BRACKET
};


TextScreen::TextScreen()
{
	// nothing to do
}


TextScreen *TextScreen::create(
	uint32_t width,
	uint32_t height,
	uint32_t depth,
	const Font &font )
{
	TextScreen *object = new TextScreen();

	ScreenInfo &info = object->info;

	info.width      = width;
	info.height     = height;
	info.depth      = depth;
	info.pitch      = width * (depth / 8);
	info.foreground = 9;
	info.background = 0;
	info.bold       = false;
	info.font       = &font;
	info.scroll     = false;
	info.columns    = width / info.font->getGlyphWidth();
	info.rows       = height / info.font->getGlyphHeight();

	info.textSize   = info.columns * info.rows;
	info.bufferSize = width * height * (depth / 8);

	info.buffer = (Color*) new uint8_t[info.textSize * 2 + info.bufferSize];
	info.text = (uint8_t*) info.buffer + info.bufferSize;
	info.attribute = info.text + info.textSize;

	memset(info.attribute, 0, info.textSize);
	memset(info.text, 0, info.textSize);
	memset(info.buffer, 0, info.bufferSize);

	info.setOffset(0);

	return object;
}


TextScreen::~TextScreen()
{
	delete[] info.buffer;
}


void TextScreen::write(
	const char16_t *text,
	size_t size,
	bool )
{
	// acquire lock

	for (; size > 0; --size, ++text)
	{
		if (*text == '\t')
			print(u"    ");
		else
			print(*text);
	}

	// release lock
}


void TextScreen::print(
	char16_t symbol )
{
	static AnsiEscapeState state = AES_NONE;
	static size_t param = AES_NONE;

	if (state == AES_NONE)
	{
		switch (symbol)
		{
			case '\33':
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
				info.text[ info.getOffset() ] = (char) symbol;
				info.attribute[ info.getOffset() ] = (uint8_t) (info.foreground | (info.background << 5));
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
				// fall through

			case ';':
				if (param == 0)
				{
					info.foreground = 9;
					info.background = 0;
					info.bold = false;
				}
				else
				if (param == 1)
				{
					info.bold = true;
				}
				if (param >= 30 && param <= 39)
				{
					info.foreground = param - 30;
					if (info.bold) info.foreground += 10;
				}
				else
				if (param >= 40 && param <= 49)
				{
					// we are ignoring the 'default color' (49) in the background because
					// background colors are encoded with 3 bits (don't fit!)
					if (param > 47)
						info.background = 0;
					else
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


void TextScreen::print(
	const char16_t *format,
	... )
{
	va_list args;
	char16_t buffer[128];
	int n = 0;

	va_start(args, format);
	n = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	write(buffer, n);
}



void TextScreen::refresh()
{
	uint32_t glyphW = info.font->getGlyphWidth();
	uint32_t glyphH = info.font->getGlyphHeight();

	size_t *ptr = (size_t*) info.buffer;
	size_t color = 	DISPLAY_PALETTE[0] << 16 | DISPLAY_PALETTE[0];
	#ifdef ARM_64
	color |= (color << 32);
	#endif

	// clean the screen
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

	for (register uint32_t y = 0; y < totalY; y += glyphH)
	{
		for (register uint32_t x = 0; x < totalX; x += glyphW)
		{
			register size_t index = start % info.textSize;
			++start;

			if (info.text[index] == ' ')
				draw(x, y, DISPLAY_PALETTE[ info.attribute[index] >> 5 ] );
			else
				draw(info.text[index], x, y,
					DISPLAY_PALETTE[ info.attribute[index] & 0x1F ],
					DISPLAY_PALETTE[ info.attribute[index] >> 5 ] );
		}
	}

}


void TextScreen::draw(
	char symbol,
	uint32_t posX,
	uint32_t posY,
	Color foreground,
	Color background )
{
	uint32_t glyphW = info.font->getGlyphWidth();
	uint32_t glyphH = info.font->getGlyphHeight();

	const uint8_t *glyph = info.font->getGlyph(symbol);

	for (register uint32_t y = 0; y < glyphH; ++y)
	{
		uint32_t offset = (y + posY) * info.width + posX;
		uint32_t glyphIndex = y *glyphW;

		for (register uint32_t x = 0; x < glyphW; ++x)
		{
			if (glyph[glyphIndex + x] != 0)
				info.buffer[ offset+ x ] = foreground;
			else
				info.buffer[ offset+ x ] = background;
		}
	}
}


void TextScreen::draw(
	uint32_t posX,
	uint32_t posY,
	Color background )
{
	uint32_t glyphW = info.font->getGlyphWidth();
	uint32_t glyphH = info.font->getGlyphHeight();

	for (register uint32_t y = 0; y < glyphH; ++y)
	{
		uint32_t offset = (y + posY) * info.width + posX;

		for (register uint32_t x = 0; x < glyphW; ++x)
		{
			info.buffer[ offset + x ] = background;
		}
	}
}


OPTIMIZE_0 void TextScreen::colorTest()
{
	static const char16_t HEADER[] = u"\33[0m         40m   41m   42m   43m   44m   45m   46m   47m\n";

	write(HEADER, (sizeof(HEADER) - 1) / sizeof(char16_t));

	for (int foreColor = 0; foreColor < 8; ++foreColor)
	{
		print(u"\33[0m    3%dm ", foreColor);
		for (int backColor = 0; backColor < 8; ++backColor)
		{
			print(u"\33[3%d;4%dm AbC \33[0m ", foreColor, backColor);
		}

		print(u"\n\33[0m  1;3%dm ", foreColor);
		for (int backColor = 0; backColor < 8; ++backColor)
		{
			print(u"\33[1;3%d;4%dm AbC \33[0m ", foreColor, backColor);
		}
		print('\n');
	}
}



} // machina