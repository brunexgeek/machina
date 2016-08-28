#ifndef MACHINA_FONT_HH
#define MACHINA_FONT_HH


#include <sys/types.h>


namespace machina {


struct FontInformation
{
	uint32_t glyphWidth;
	uint32_t glyphHeight;
	uint32_t glyphCount;
	uint8_t glyphData[256][16];
};


class Font
{
	public:
		static Font &getInstance();

		const uint8_t *getGlyph(
			uint32_t code ) const;

		size_t getGlyphHeight() const;

		size_t getGlyphWidth() const;

		bool getBitUnsafe(
			uint32_t code,
			uint32_t posX,
			uint32_t posY ) const
		{
			const uint8_t *bits = getGlyph(code);
			return bits[posY] & (1 << (7 - posX));
		}
};


} // machina

#endif // MACHINA_FONT_HH