#ifndef MACHINA_MAILBOX_H
#define MACHINA_MAILBOX_H


#include <sys/types.h>


#define MAILBOX_CODE_REQUEST           (0x00000000)
#define MAILBOX_CODE_RESPONSE_OK       (0x80000000)
#define MAILBOX_CODE_RESPONSE_PARTIAL  (0x80000001)
#define MAILBOX_RESPONSE_BIT           (1 << 31)


typedef struct
{
	uint32_t tag;
	uint32_t bufferSize;
	uint32_t valueLength;
} mailbox_tag_t;


typedef struct
{
	mailbox_tag_t header;
	uint32_t base;
	uint32_t size;
} memory_tag_t;


uint32_t mailbox_send(
	uint32_t channel,
	uint32_t request );

bool mailbox_getProperty(
	uint32_t channel,
	uint32_t tag,
	void *data,
	uint32_t dataSize,
	uint32_t *expectedSize = nullptr );


#endif // MACHINA_MAILBOX_H