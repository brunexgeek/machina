#ifndef MACHINA_DISPLAY_HH
#define MACHINA_DISPLAY_HH


#include <sys/types.h>
#include <sys/Font.hh>
#include <sys/Device.hh>
#ifndef __arm__
#include <iostream>
#endif

#define MAKE_COLOR(r, g, b) \
	( ( ( (r) >> 3 ) << 11) | ( ( (g) >> 2 ) << 5) | ( (b) >> 3 ) )


namespace machina {


#define DISPLAY_WIDTH        (800)
#define DISPLAY_HEIGHT       (600)
#define DISPLAY_DEPTH        (sizeof(Color) * 8)


typedef uint16_t Color;


class TextScreen;


class Display : public Device
{
	public:
		Display (
			uint32_t width = DISPLAY_WIDTH,
			uint32_t height = DISPLAY_HEIGHT,
			uint32_t depth = DISPLAY_DEPTH );

		~Display();

		const char* getName() const;

		const char* getFileName() const;

		static Display &getInstance();

		bool Initialize (void);

		uint32_t getWidth () const
		{
			return width;
		}

		uint32_t getHeight () const
		{
			return height;
		}

		uint32_t getDepth () const
		{
			return depth;
		}

		void refresh();

		void print(
			const char *text,
			bool overwrite = true );

		void clearLine();

		void print(
			char symbol );

		void draw(
			const char *text,
			uint32_t posX,
			uint32_t posY,
			const Font &font,
			Color foreground,
			Color background );

		void draw(
			char symbol,
			uint32_t posX,
			uint32_t posY,
			const Font &font,
			Color foreground,
			Color background );

		void draw(
			const TextScreen &screen );

	private:
		Color *buffer;
		size_t bufferSize;
		uint32_t width;
		uint32_t height;
		uint32_t depth;   // only 16-bits for now


};


} // machina



#endif // MACHINA_DISPLAY_HH