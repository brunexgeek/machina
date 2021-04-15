#include <sys/mailbox.h>
#include <sys/sync.h>
#include <sys/bcm2837.h>
#include <sys/sysio.h>
#include <mc/string.h>

#define MAILBOX ((volatile __attribute__((aligned(4))) struct mailbox_memory_t*)(uintptr_t)(SOC_MAILBOX_BASE))
#define MAIL_EMPTY	0x40000000  // nailbox empty
#define MAIL_FULL	0x80000000  // mailbox full

struct __attribute__((__packed__, aligned(4))) mailbox_memory_t
{
	const uint32_t Read0;											// 0x00         read data
	uint32_t Unused[3];												// 0x04-0x0F
	uint32_t Peek0;													// 0x10
	uint32_t Sender0;												// 0x14
	uint32_t Status0;												// 0x18         read status
	uint32_t Config0;												// 0x1C
	uint32_t Write1;												// 0x20         write data
	uint32_t Unused2[3];											// 0x24-0x2F
	uint32_t Peek1;													// 0x30
	uint32_t Sender1;												// 0x34
	uint32_t Status1;												// 0x38         write status
	uint32_t Config1;												// 0x3C
};

bool mailbox_write( MAILBOX_CHANNEL channel, uint32_t addr )
{
	if ((channel >= 0) && (channel <= MB_CHANNEL_GPU))
	{
		// ensure the channel is set
		addr = (addr & ~(0xFU)) | channel;
		// wait for some space in the mailbox
		while ((MAILBOX->Status1 & MAIL_FULL) != 0);
		MAILBOX->Write1 = addr;
		return true;
	}
	return false;
}

bool mailbox_read( MAILBOX_CHANNEL channel )
{
	if ((channel >= 0) && (channel <= MB_CHANNEL_GPU))
	{
		uint32_t value;
		do {
			// wait for data
			while ((MAILBOX->Status0 & MAIL_EMPTY) != 0);
			value = MAILBOX->Read0;
		} while ((value & 0xFU) != channel);
		return true;
	}
	return false;
}

int mailbox_tag( uint32_t tag , struct mailbox_message *buffer )
{
	// manually align the memory because the 'align' attribute wont work with aarch64 (why?)
	uint32_t tmp[sizeof(struct mailbox_message) + 15];
	uint32_t addr = ((uint32_t) (size_t) &tmp + 15) & (~15U);
	struct mailbox_message *message = (struct mailbox_message *) (size_t) addr;

	memset(tmp, 0, sizeof(tmp));
	message->size = sizeof(struct mailbox_message);
	message->tag.header.id = tag;
	message->tag.header.size = sizeof(struct mailbox_message) - 4 - 4 - 4 - sizeof(struct mailbox_tag_header);

	__asm volatile ("dc civac, %0" : : "r" (addr) : "memory");

	mailbox_write(MB_CHANNEL_TAGS, GPU_MEMORY_BASE | addr);
	mailbox_read(MB_CHANNEL_TAGS);

	__asm volatile ("dc civac, %0" : : "r" (addr) : "memory");

	if (message->code == MAILBOX_CODE_RESPONSE_OK)
	{
		if (buffer)
			memcpy(buffer, message, sizeof(struct mailbox_message));
		return 0;
	}
	return message->code & (~MAILBOX_RESPONSE_BIT);
}