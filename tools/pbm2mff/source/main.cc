#include <sys/Font.hh>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>


using machina::Font;
using namespace std;


struct PBM
{
	uint32_t width;
	uint32_t height;
	uint32_t glyphs;
	uint8_t *data;
};


int exportText()
{
	Font &font = Font::getInstance();
	size_t width = font.getGlyphWidth();
	size_t height = font.getGlyphHeight();

	std::cout << "P1" << std::endl;
	std::cout << width << ' ' << height * 256 << std::endl;

	for (size_t i = 0x00; i <= 0xFF; ++i)
	{
		const uint8_t *glyph = font.getGlyph(i);

		std::cout << "# " << i << std::endl;

		for (size_t y = 0; y < height; ++y)
		{
			for (int x = width - 1; x >= 0; --x)
				std::cout << ( (glyph[y] & (1 << x)) > 0 );
			std::cout << std::endl;
		}
	}
	return 0;
}


bool importPBM(
	const std::string &fileName,
	PBM &pbm )
{
	std::ifstream input(fileName.c_str(), std::ios_base::binary);
	if (!input.good()) return false;

	int row = 0, col = 0, numrows = 0, numcols = 0;
	stringstream ss;
	string line = "";

	// First line : version
	getline(input, line);
	if (line.compare("P4") != 0)
		return false;

	do
	{
		getline(input, line);
		if (!line.empty() && line[0] != '#') break;
	} while (true);

	ss << line;
	ss >> pbm.width >> pbm.height;
	pbm.glyphs = (pbm.height / (pbm.width * 2));

	std::cerr << "Monochrome PBM " << std::endl;
	std::cerr << "   Width: " << pbm.width << std::endl;
	std::cerr << "  Height: " << pbm.height << std::endl;
	std::cerr << "  Glyphs: " << pbm.glyphs << std::endl;

	if (pbm.width != 8 || pbm.height != 4096) return false;

	do
	{
		char current = input.get();
		if (current != 0x0D && current != 0x0A && current != 0x20)
			break;
	} while (true);

	pbm.data = new uint8_t[pbm.height * pbm.width]();

	input.read( (char*) pbm.data, pbm.height * pbm.width);

	input.close();

	return true;
}


void printGlyph(
	uint8_t *glyph,
	size_t size )
{
	std::cout << "\t\t{ " << hex << setfill('0');

	for (size_t i = 0; i < size; ++i)
		std::cout << "0x" << setw(2) << (int) glyph[i] << ", ";

	std::cout << "}," << std::endl;
}


void exportPBM(
	PBM &pbm )
{
	uint32_t glyphSize = 16; // 8 bits x 16 rows
	const uint8_t *unknownGlyph = pbm.data;

	std::cout << "FontInformation FONT_DATA = " << std::endl << "{" << std::endl;
	std::cout << "\t" << pbm.width << ", " << (pbm.width * 2) << ", " << 255 << "," << std::endl;
	std::cout << "\t{ " << std::endl;

	for (size_t i = 0; i < 255; ++i)
	{
		printGlyph( pbm.data + i * glyphSize, glyphSize);
	}

	std::cout << "\t}" << std::endl << "};" << std::endl;
}


int main( int argc, char **argv )
{
	if (argc == 2)
	{
		PBM pbm;
		if (importPBM(argv[1], pbm))
			exportPBM(pbm);
	}
	return 0;
}