#ifndef MACHINA_FONT_HH
#define MACHINA_FONT_HH


#include <sys/types.h>


namespace machina {


struct FontInformation
{
	uint32_t glyphWidth;
	uint32_t glyphHeight;
	uint32_t glyphCount;
	uint16_t glyphData[256][16];
};



class Font
{
	public:
		Font(
			const FontInformation *info );

		~Font();

		static Font &getMonospaceFont();

		static Font &getConsoleFont();

		const uint16_t *getGlyph(
			uint32_t code ) const
		{
			if (code > 0xFF) code = 0;

			return info->glyphData[code];
		}

		size_t getGlyphHeight() const
		{
			return info->glyphHeight;
		}

		size_t getGlyphWidth() const
		{
			return info->glyphWidth;
		}

	private:
		const FontInformation *info;
};


} // machina

#endif // MACHINA_FONT_HH