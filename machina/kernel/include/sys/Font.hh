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
			uint32_t code ) const;

		size_t getGlyphHeight() const;

		size_t getGlyphWidth() const;

	private:
		const FontInformation *info;
};


} // machina

#endif // MACHINA_FONT_HH