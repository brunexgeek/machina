#include <sys/Mailbox.hh>
#include <sys/Timer.hh>
#include <sys/sync.h>
#include <sys/soc.h>
#include <sys/sysio.hh>
#include <sys/stdlib.hh>


namespace machina {


struct MailboxBuffer
{
	uint32_t size;
	uint32_t code;
	uint8_t tags[0];
};



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


/*
 * Returns the value for a mailbox property (only first tag).
 *
 * The @c data argument is for input/output. This pointer it's supposed to be
 * a memory region with at least @c sizeof(MailboxTag).
 *
 * This function retrieves mailbox properties as specified at
 * https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 */
bool Mailbox::getProperty(
	uint32_t channel,
	uint32_t tagId,
	void *data,
	uint32_t dataSize,
	uint32_t *expectedSize )
{
	uint32_t alignedDataSize = (dataSize + 3) & ~0x03;

	// computes the mailbox buffer: mailbox buffer struct + aligned data size + end tag
	uint32_t bufferSize = sizeof(MailboxBuffer) + alignedDataSize + sizeof(uint32_t);

	// Note: at this point it's not safe to call dynamic memory allocation (physical
	//       memory manager could not be initialized yet).

	// "allocates" some memory for the request/response buffer (we need to be sure
	// this buffer is 16 bytes aligned, so we allocate some extra bytes to have room
	// for adjustments)
	uint8_t temp[bufferSize + 15];
	// prepare the mailbox buffer (16 bytes aligned)
	MailboxBuffer *buffer = (MailboxBuffer*) (((uint32_t) temp + 15) & ~(15));
	buffer->size = bufferSize;
	buffer->code = MAILBOX_CODE_REQUEST;
	memcpy(buffer->tags, data, dataSize);
	// prepare the mailbox tag
	MailboxTag *tag = (MailboxTag *) buffer->tags;
	tag->tag         = tagId;
	tag->bufferSize  = alignedDataSize - sizeof(MailboxTag);
	tag->valueLength = 0;

	// TODO: remove tag header from "data". The "data" should be the value buffer only.

	// Note: Because the tag data is 4 bytes aligned tag, in some cases, we are providing
	//       more space than necessary for tags (this means that value buffer size could
	//       be greater than value length).

	// Note: If the given value buffer size does not have enought space to store the data
	//       the response code will be MAILBOX_CODE_RESPONSE_PARTIAL. In this case,
	//       the value buffer contains a truncated version of the data and the value length
	//       contains the expected value buffer size.

	*((uint32_t*) (buffer->tags + alignedDataSize)) = 0;

	uint32_t address = GPU_MEM_BASE + (uint32_t) buffer;
	if (send(channel, address) != address)
	{
		return false;
	}

	sync_dataMemBarrier();

	// returns the expected value buffer size
	if (expectedSize != NULL)
		*expectedSize = tag->valueLength & ~MAILBOX_RESPONSE_BIT;

	// if the property is invalid, we fail (and expected size is probably zero)
	if ((tag->valueLength & MAILBOX_RESPONSE_BIT) == 0)
		return false;

	memcpy(data, buffer->tags, dataSize);

	// if the response is partial, we kind a fail
	return (buffer->code != MAILBOX_CODE_RESPONSE_OK);
}



} // machina