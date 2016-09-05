#ifndef MACHINA_DISPLAY_HH
#define MACHINA_DISPLAY_HH


#include <sys/types.h>
#include <sys/Font.hh>
#include <sys/Device.hh>
#ifndef __arm__
#include <iostream>
#endif

#define DISPLAY_COLOR(r, g, b) \
	( ( ( (r) >> 3 ) << 11) | ( ( (g) >> 2 ) << 5) | ( (b) >> 3 ) )


namespace machina {


#define DISPLAY_WIDTH        (800)
#define DISPLAY_HEIGHT       (600)
#define DISPLAY_DEPTH        (16) // 16 bits
#define DISPLAY_SIZE         (DISPLAY_WIDTH * DISPLAY_HEIGHT)
#define DISPLAY_COLUMNS      (DISPLAY_WIDTH / 8)
#define DISPLAY_ROWS         (DISPLAY_HEIGHT / 16)


typedef uint16_t Color;


struct DisplayInfo
{
	Color *buffer;
	uint32_t bufferSize;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
	uint32_t columns;
	uint32_t rows;
	uint32_t nState;
	uint32_t foreground;
	uint32_t background;
	const Font *font;
	uint8_t text[DISPLAY_COLUMNS * DISPLAY_ROWS];
	uint32_t textSize;
	uint32_t _textOffset;
	/*
	 * 01-03  Color index (16 colors)
	 * 04-07  Unused
	 */
	uint8_t attribute[DISPLAY_WIDTH * DISPLAY_HEIGHT];
	uint32_t lineMask[8];
	bool scroll;

	void setOffset(
		uint32_t offset )
	{
		_textOffset = offset;


		// every time we reach the end of the line we need to
		// clean the next one
		if ((_textOffset % (columns - 1)) == 0)
		{
			for (size_t i = _textOffset + 1; i < _textOffset + 1 + columns; ++i)
				text[i] = ' ';
		}

		if (_textOffset > textSize)
		{
			scroll = true;
			_textOffset %= textSize;
		}
	}

	uint32_t getOffset()
	{
		return _textOffset;
	}
};


class Display : public Device
{
	public:
		Display (
			uint32_t width = DISPLAY_WIDTH,
			uint32_t height = DISPLAY_HEIGHT );

		~Display();

		const char* getName() const;

		const char* getFileName() const;

		//static Display &getInstance();

		bool Initialize (void);

		uint32_t getWidth () const
		{
			return info.width;
		}

		uint32_t getHeight () const
		{
			return info.height;
		}

		uint32_t getColumns () const
		{
			return info.columns;
		}
		uint32_t getRows () const
		{
			return info.rows;
		}

		const Font &getFont() const
		{
			return *info.font;
		}

		const DisplayInfo &getInfo() const
		{
			return info;
		}

		void refresh();

		void print(
			const char *text,
			bool overwrite = true );

		void clearLine();

		void print(
			char symbol );


		template <typename T>
		void printHex(
			T value )
		{
			static const char TEXT[] = "0123456789abcdef";

			print("0x");

			for (int shift = sizeof(T) * 8 - 4; shift >= 0; shift -= 4)
				print( TEXT[ (value >> shift) & 0x0F ] );
		}

	private:
		DisplayInfo info;

		void draw(
			char symbol,
			uint32_t posX,
			uint32_t posY,
			Color foreground,
			Color background );
};


} // machina



#endif // MACHINA_DISPLAY_HH