#ifndef MACHINA_FONT_HH
#define MACHINA_FONT_HH


#include <sys/types.h>


namespace machina {


struct FontInformation
{
	uint32_t glyphWidth;
	uint32_t glyphHeight;
	uint32_t glyphCount;
	const uint8_t *glyphData;
};



class Font
{
	public:
		~Font();

		const uint8_t *getGlyph(
			uint32_t code ) const
		{
			if (code > 0xFF) code = 0;

			return glyphData + code * glyphSize;
		}

		size_t getGlyphHeight() const
		{
			return glyphHeight;
		}

		size_t getGlyphWidth() const
		{
			return glyphWidth;
		}

		static const Font *load(
			const uint8_t *buffer,
			size_t size );

	private:
		uint32_t glyphWidth;
		uint32_t glyphHeight;
		uint32_t glyphCount;
		uint32_t glyphSize;
		uint8_t *glyphData;

		Font();

		static const Font *loadPsf1(
			const uint8_t *buffer,
			size_t size );

		static const Font *loadPsf2(
			const uint8_t *buffer,
			size_t size );
};


} // machina

#endif // MACHINA_FONT_HH