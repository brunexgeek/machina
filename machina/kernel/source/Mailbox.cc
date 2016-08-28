#include <sys/Mailbox.hh>
#include <sys/Timer.hh>
#include <sys/sync.h>
#include <sys/soc.h>
#include <sys/sysio.hh>


namespace machina {


Mailbox::Mailbox ()
{
	// nothing to do
}


Mailbox::~Mailbox ()
{
	// nothing to do
}


uint32_t Mailbox::send(
	uint32_t channel,
	uint32_t request )
{
	uint32_t result;

	sync_dataMemBarrier();

	// acquire global lock

	// write the request and retrieve the response
	write(channel, request);
	result = read(channel);

	// release global lock

	sync_dataMemBarrier();

	return result;
}


uint32_t Mailbox::read(
	uint32_t channel )
{
	uint32_t result;

	do
	{
		// wait until the mailbox is full
		while ((GET32(SOC_MAILBOX_POLL) & SOC_MAILBOX_STATUS_EMPTY) != 0);

		// check whether the response is for the desired channel
		result = GET32(SOC_MAILBOX_READ);
	} while ( (result & 0x0000000F) != channel );

	return result & 0xFFFFFFF0;
}


void Mailbox::write(
	uint32_t channel,
	uint32_t request )
{
	// wait until the mailbox is empty
	while ((GET32(SOC_MAILBOX_POLL) & SOC_MAILBOX_STATUS_FULL) != 0);

	request &= 0xFFFFFFF0;
	PUT32(SOC_MAILBOX_WRITE, request | channel);
}


} // machina