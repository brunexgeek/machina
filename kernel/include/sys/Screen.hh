#ifndef MACHINA_SCREEN_HH
#define MACHINA_SCREEN_HH


#include <sys/types.h>
#include <sys/Font.hh>


namespace machina {


struct ScreenInfo
{
	Color *buffer;
	uint32_t bufferSize;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t columns;
	uint32_t rows;
	uint32_t foreground;
	uint32_t background;
	const Font *font;
	uint8_t *text;
	uint32_t textSize;
	uint32_t _textOffset;
	/*
	 * 01-03  Color index (16 colors)
	 * 04-07  Unused
	 */
	uint8_t *attribute;
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


class TextScreen
{
	friend class Display;

	public:
		static TextScreen *create(
			uint32_t width,
			uint32_t height,
			uint32_t depth );

		~TextScreen();

		template <typename T>
		void printHex(
			T value )
		{
			static const char TEXT[] = "0123456789abcdef";

			print("0x");

			for (int shift = sizeof(T) * 8 - 4; shift >= 0; shift -= 4)
				print( TEXT[ (value >> shift) & 0x0F ] );
		}

		void print(
			const char *text,
			bool overwrite = true );

		void print(
			char text );

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

		void refresh();

		void draw(
			char symbol,
			uint32_t posX,
			uint32_t posY,
			Color foreground,
			Color background );

	private:
		ScreenInfo info;

		TextScreen();
};


} // machina


#endif // MACHINA_SCREEN_HH