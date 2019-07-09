#ifndef MACHINA_MAILBOX_H
#define MACHINA_MAILBOX_H


#include <sys/types.h>


#define MAILBOX_CODE_REQUEST           (0x00000000U)
#define MAILBOX_CODE_RESPONSE_OK       (0x80000000U)
#define MAILBOX_CODE_RESPONSE_PARTIAL  (0x80000001U)
#define MAILBOX_RESPONSE_BIT           (1U << 31)


struct mailbox_header
{
	uint32_t tag;
	uint32_t bufferSize;
	uint32_t valueLength;
} ;

struct memory_tag
{
	struct mailbox_header header;
	uint32_t base;
	uint32_t size;
};

struct mac_tag
{
	struct mailbox_header tag;
	uint8_t address[6];
	uint8_t padding[2];
};


#ifdef __cplusplus
extern "C" {
#endif

uint32_t mailbox_send(
	uint32_t channel,
	uint32_t request );

bool mailbox_getProperty(
	uint32_t channel,
	uint32_t tag,
	void *data,
	uint32_t dataSize,
	uint32_t *expectedSize);

#ifdef __cplusplus
}
#endif


#endif // MACHINA_MAILBOX_H