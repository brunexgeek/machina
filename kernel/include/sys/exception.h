#ifndef MACHINA_EXCEPTION_H
#define MACHINA_EXCEPTION_H


#include <sys/types.h>


#define EXCP_DIV0             0
#define EXCP_UNDEF_INST       1
#define EXCP_PREF_ABORT       2
#define EXCP_DATA_ABORT       3
#define EXCP_UNKNOWN          4


struct excepttbl
{
	uint32_t UndefinedInstruction;
	uint32_t SupervisorCall;
	uint32_t PrefetchAbort;
	uint32_t DataAbort;
	uint32_t Unused;
	uint32_t IRQ;
	uint32_t FIQ;
};


#ifdef __cplusplus
extern "C" {
#endif

int except_initialize();

#ifdef __cplusplus
}
#endif


#endif // MACHINA_EXCEPTION_H
