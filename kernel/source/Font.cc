#include <sys/Font.hh>


namespace machina {


#include "console-8x16.inc"
#include "monospace-10x16.inc"


static Font monospace(&MONOSPACE_DATA);

static Font console(&CONSOLE_DATA);


Font::Font(
	const FontInformation *info ) : info(info)
{
	// nothing to do
}


Font::~Font()
{
	// nothing to do
}


Font &Font::getMonospaceFont()
{
	return monospace;
}


Font &Font::getConsoleFont()
{
	return console;
}

/*
const uint16_t *Font::getGlyph(
	uint32_t code ) const
{
	if (code > 0xFF) code = 0;

	return info->glyphData[code];
}


size_t Font::getGlyphHeight() const
{
	return info->glyphHeight;
}


size_t Font::getGlyphWidth() const
{
	return info->glyphWidth;
}*/



}