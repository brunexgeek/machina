#ifndef MACHINA_MAILBOX_H
#define MACHINA_MAILBOX_H


#include <sys/types.h>


namespace machina {


class Mailbox
{
	public:
		~Mailbox();

		static uint32_t send(
			uint32_t channel,
			uint32_t request );

	private:
		Mailbox();

		static void waitAvailable();

		static uint32_t read(
			uint32_t channel );

		static void write(
			uint32_t channel,
			uint32_t request );
};


} //machina


#endif // MACHINA_MAILBOX_H