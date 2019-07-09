#include <sys/mailbox.h>
#include <sys/sync.h>
#include <sys/soc.h>
#include <sys/sysio.h>
#include <sys/heap.h>
#include <mc/string.h>


struct mailbox_buffer
{
	uint32_t size;
	uint32_t code;
	uint8_t tags[0];
};


static uint32_t mailbox_read(
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


static void mailbox_write(
	uint32_t channel,
	uint32_t request )
{
	// wait until the mailbox is empty
	while ((GET32(SOC_MAILBOX_POLL) & SOC_MAILBOX_STATUS_FULL) != 0);

	request &= 0xFFFFFFF0;
	PUT32(SOC_MAILBOX_WRITE, request | channel);
}


uint32_t mailbox_send(
	uint32_t channel,
	uint32_t request )
{
	uint32_t result;

	//sync_dataMemBarrier();
	// acquire global lock

	// write the request and retrieve the response
	mailbox_write(channel, request);
	result = mailbox_read(channel);

	// release global lock
	//sync_dataMemBarrier();

	return result;
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
bool mailbox_getProperty(
	uint32_t channel,
	uint32_t tagId,
	void *data,
	uint32_t dataSize,
	uint32_t *expectedSize )
{
	uint32_t alignedDataSize = (dataSize + 3) & ~3U;

	// computes the mailbox buffer: mailbox buffer struct + aligned data size + end tag
	uint32_t bufferSize = sizeof(struct mailbox_buffer) + alignedDataSize + sizeof(uint32_t);

	// Note: at this point it's not safe to call dynamic memory allocation (physical
	//       memory manager could not be initialized yet).

	// "allocates" some memory for the request/response buffer (we need to be sure
	// this buffer is 16 bytes aligned, so we allocate some extra bytes to have room
	// for adjustments)
	void *temp = heap_allocate(bufferSize + 15);
	// prepare the mailbox buffer (16 bytes aligned)
	struct mailbox_buffer *buffer = (struct mailbox_buffer*) (((size_t) temp + 15) & ~15U);
	buffer->size = bufferSize;
	buffer->code = MAILBOX_CODE_REQUEST;
	memcpy(buffer->tags, data, dataSize);
	// prepare the mailbox tag
	struct mailbox_header *tag = (struct mailbox_header *) buffer->tags;
	tag->tag         = tagId;
	tag->bufferSize  = alignedDataSize - sizeof(struct mailbox_header);
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

	uint32_t address = GPU_MEMORY_BASE + (uint32_t) buffer;
	if (mailbox_send(channel, address) != address)
	{
		heap_free(temp);
		return false;
	}

	//sync_dataMemBarrier();

	// returns the expected value buffer size
	if (expectedSize != NULL)
		*expectedSize = tag->valueLength & ~MAILBOX_RESPONSE_BIT;

	// if the property is invalid, we fail (and expected size is probably zero)
	if ((tag->valueLength & MAILBOX_RESPONSE_BIT) == 0)
		return false;

	memcpy(data, buffer->tags, dataSize);
	heap_free(temp);

	// if the response is partial, we kind a fail
	return (buffer->code != MAILBOX_CODE_RESPONSE_OK);
}
