#ifndef MACHINA_DISPLAY_HH
#define MACHINA_DISPLAY_HH


#include <sys/types.h>
#include <sys/Font.hh>
#include <sys/Device.hh>
#ifndef __arm__
#include <iostream>
#endif


namespace machina {


#define DISPLAY_WIDTH        (1024)
#define DISPLAY_HEIGHT       (768)
#define DISPLAY_DEPTH        (sizeof(Color) * 8)


typedef uint16_t Color;


class TextScreen;


class Display : public Device
{
	public:
		~Display();

		const char* getName() const
		{
			return "VideoCore IV";
		}

		const char*getFileName() const
		{
			return "display";
		}

		static Display &getInstance();

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

		const void *getBuffer() const
		{
			return buffer;
		}

		uint32_t getBufferSize() const
		{
			return bufferSize;
		}

		void print(
			const char *text,
			bool overwrite = true );

		void print(
			char symbol );

		void draw(
			char symbol,
			uint32_t posX,
			uint32_t posY,
			const Font &font,
			Color foreground,
			Color background );

		void draw(
			const TextScreen &screen,
			int32_t x = 0,
			int32_t y = 0 );

	private:
		Color *buffer;
		size_t bufferSize;
		uint32_t width;
		uint32_t height;
		uint32_t depth;   // only 16-bits for now
		uint32_t pitch;

		Display (
			uint32_t width = DISPLAY_WIDTH,
			uint32_t height = DISPLAY_HEIGHT,
			uint32_t depth = DISPLAY_DEPTH );
};


} // machina



#endif // MACHINA_DISPLAY_HH