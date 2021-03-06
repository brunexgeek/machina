#ifndef MACHINA_PROCFS_H
#define MACHINA_PROCFS_H


#include <sys/types.h>
#include <sys/vfs.h>


//typedef int (*procfunc_t)( struct file *fp, void *data );
typedef int (*procfunc_t)( uint8_t *buffer, int size, void *data );


#ifdef __cplusplus
extern "C" {
#endif

int procfs_initialize();

int procfs_register( const char *name, procfunc_t func, void *data );

int procfs_unregister( const char *name );

#ifdef __cplusplus
}
#endif


#endif // MACHINA_PROCFS_H