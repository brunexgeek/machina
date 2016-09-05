#ifndef MACHINA_SYNC_H
#define MACHINA_SYNC_H


#ifdef __cplusplus
extern "C" {
#endif


#define	sync_enableInterrupts()	\
	asm volatile ("cpsie i")

#define	sync_disableInterrupts() \
	asm volatile ("cpsid i")


#if (RPIGEN == 1)

#define sync_dataSyncBarrier() \
	asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0) : "memory")

#define sync_dataMemBarrier() \
	asm volatile ("mcr p15, 0, %0, c7, c10, 5" : : "r" (0) : "memory")

#define sync_instSyncBarrier() \
	asm volatile ("mcr p15, 0, %0, c7, c5,  4" : : "r" (0) : "memory")

#define sync_instMemBarrier() \
	sync_instSyncBarrier()

#else

#define sync_dataSyncBarrier() \
	asm volatile ("dsb" ::: "memory")

#define sync_dataMemBarrier() \
	asm volatile ("dmb" ::: "memory")

#define sync_instSyncBarrier() \
	asm volatile ("isb" ::: "memory")

#define sync_instMemBarrier() \
	asm volatile ("isb" ::: "memory")

#endif


#ifdef __cplusplus
}
#endif


#endif // MACHINA_SYNC_H