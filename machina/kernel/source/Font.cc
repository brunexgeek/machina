#include <sys/Font.hh>


namespace machina {


#include "console-8x16.inc"


static Font instance;


Font &Font::getInstance()
{
	return instance;
}


const uint8_t *Font::getGlyph(
	uint32_t code ) const
{
	if (code > 0xFF)
		code = 0;

	return FONT_DATA.glyphData[code];
}


size_t Font::getGlyphHeight() const
{
	return FONT_DATA.glyphHeight;
}


size_t Font::getGlyphWidth() const
{
	return FONT_DATA.glyphWidth;
}



}