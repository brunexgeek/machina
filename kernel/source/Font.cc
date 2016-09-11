#include <sys/Font.hh>
#include <mc/string.h>

#ifndef __arm__
#include <iostream>
#endif

namespace machina {


#define PSF1_MAGIC          (0x0436)
#define PSF1_MODE512        (0x01)
#define PSF1_MODEHASTAB     (0x02)
#define PSF1_MODEHASSEQ     (0x04)
#define PSF1_MAXMODE        (0x05)

#define PSF1_SEPARATOR      (0xFFFF)
#define PSF1_STARTSEQ       (0xFFFE)


#define PSF2_MAGIC          (0x864AB572U)

struct psf2_header
{
	uint8_t magic[4];
	uint32_t version;
	uint32_t headersize;    /* offset of bitmaps in file */
	uint32_t flags;
	uint32_t length;        /* number of glyphs */
	uint32_t charsize;      /* number of bytes for each character */
	uint32_t height, width; /* max dimensions of glyphs */
	/* charsize = height * ((width + 7) / 8) */
};


Font::Font(
	/*const FontInformation &info*/ )
{
	/*glyphWidth  = info.glyphWidth;
	glyphHeight = info.glyphHeight;
	glyphSize   = ((glyphWidth + 0x07) & ~0x07)  / 8 * glyphHeight;
	glyphCount  = info.glyphCount;

	bitmap = new uint8_t[glyphCount * glyphSize];
	mc_memcpy(bitmap, info.glyphData, glyphCount * glyphSize);*/
}


Font::~Font()
{
	delete[] glyphData;
}

/*
Font &Font::getMonospaceFont()
{
	return monospace;
}


Font &Font::getConsoleFont()
{
	return console;
}
*/

const Font *Font::load(
	const uint8_t *buffer,
	size_t size )
{
	if (buffer == nullptr || size == 0)
		return nullptr;

	if ( *((uint16_t*) buffer) == PSF1_MAGIC )
		return loadPsf1(buffer, size);
	else
	if ( *((uint32_t*) buffer) == PSF2_MAGIC )
		return loadPsf2(buffer, size);

	return nullptr;
}


static void bitmapToBytemap(
	const uint8_t *bitmap,
	uint32_t glyphWidth,
	uint32_t glyphHeight,
	uint32_t glyphCount,
	uint8_t *output )
{
	size_t step = ((glyphWidth + 7) & ~7) / 8; // bytes per width
	size_t glyphSize = step * glyphHeight;     // bytes per glyph

	const uint8_t *end = bitmap + glyphCount * glyphSize;
	for (; bitmap < end; bitmap += step)
	{
		uint16_t value;

		if (step == 1)
		{
			value = *bitmap;
		}
		else
		{
			value = *(uint16_t*)bitmap;
			value = (uint16_t) (value >> 8 | value << 8);
		}

		size_t i = 1 << (step * 8 - 1);
		size_t j = glyphWidth;
		for (; i > 0 && j > 0; --j, i >>= 1)
		{
			*output++ = (uint8_t) ( (value & i) != 0 );
		}
	}
}


const Font *Font::loadPsf1(
	const uint8_t *buffer,
	size_t /* size */ )
{
	Font *font = new Font();

	font->glyphWidth  = 8;
	font->glyphHeight = buffer[3];
	font->glyphCount  = (buffer[2] & PSF1_MODE512) ? 512 : 256;
	font->glyphSize   = font->glyphWidth * font->glyphHeight;
	font->glyphData   = new uint8_t[font->glyphSize * font->glyphCount];

	// converts the bitmap to a bytemap
	bitmapToBytemap(buffer + 4, font->glyphWidth, font->glyphHeight,
		font->glyphCount, font->glyphData);

	return font;
}


const Font *Font::loadPsf2(
	const uint8_t *buffer,
	size_t /* size */ )
{
	psf2_header *header = (psf2_header*) buffer;
	Font *font = new Font();

	font->glyphWidth  = header->width;
	font->glyphHeight = header->height;
	font->glyphCount  = header->length;
	font->glyphSize   = font->glyphWidth * font->glyphHeight;
	font->glyphData   = new uint8_t[font->glyphSize * font->glyphCount];

	// converts the bitmap to a bytemap
	bitmapToBytemap(buffer + header->headersize, font->glyphWidth,
		font->glyphHeight, font->glyphCount, font->glyphData);

	return font;
}


/*
size_t Font::getGlyphHeight() const
{
	return info->glyphHeight;
}


size_t Font::getGlyphWidth() const
{
	return info->glyphWidth;
}*/



}