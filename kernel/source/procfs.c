#include <sys/procfs.h>
#include <sys/vfs.h>
#include <sys/errors.h>
#include <mc/string.h>
#include <sys/heap.hh>
#include <sys/uart.h>


struct inode
{
    char16_t name[MAX_FILENAME];
    ino_t inode;
    procfunc_t callback;
    void *data;
    struct inode *next;
};

static struct inode *procList = NULL;

static ino_t counter = 0;

int procfs_open( struct file *fp, const char16_t *path, uint32_t flags )
{
    struct inode *inode = procList;
    while (inode)
    {
        if (strcmp(inode->name, path) == 0) break;
        inode = inode->next;
    }
    if (inode == NULL) return ENOENT;
    fp->fsdata = inode;

    // run the callback
    inode->callback(fp, inode->data);

    return EOK;
}

int procfs_close( struct file *fp )
{
    return EOK;
}

int procfs_read( struct file *fp, uint8_t *buffer, size_t count )
{
    uart_print(u"Reading '%s'\n", ((struct inode*)fp->fsdata)->name);
    return ENOIMP;
}

int procfs_write( struct file *fp, const uint8_t *buffer, size_t count )
{
    return ENOIMP;
}

int procfs_stat( struct file *fp, struct stat *info )
{
    return ENOIMP;
}

int procfs_enumerate( struct file *fp, struct dirent *entry )
{
    return ENOIMP;
}

int procfs_mount( struct mount *mp, const char16_t *opts )
{
    uart_print(u"Called '%s'\n", u"procfs_mount");
    return EOK;
}

int procfs_unmount( struct mount *mp )
{
    uart_print(u"Called '%s'\n", u"procfs_unmount");
    return EOK;
}


int procfs_initialize()
{
    static struct filesystem fs;
    strcpy(fs.name, u"procfs");
    fs.ops.open = procfs_open;
    fs.ops.close = procfs_close;
    fs.ops.read = procfs_read;
    fs.ops.write = procfs_write;
    fs.ops.stat = procfs_stat;
    fs.ops.enumerate = procfs_enumerate;
    fs.ops.mount = procfs_mount;
    fs.ops.unmount = procfs_unmount;

    return vfs_register(&fs);
}

int procfs_register( const char16_t *name, procfunc_t func, void *data )
{
    if (strlen(name ) >= MAX_FILENAME) return ETOOLONG;

    struct inode *ptr = (struct inode*) heap_allocate( sizeof(struct inode) );
    if (ptr == NULL) return EMEMORY;

    strcpy(ptr->name, name);
    ptr->callback = func;
    ptr->data = data;
    ptr->inode = ++counter;
    ptr->next = procList;
    procList = ptr;

    uart_print(u"Registered procfs file '%s'\n", name);

    return EOK;
}

int procfs_unregister( const char16_t name )
{
    // TODO: release 'inode'
    return EOK;
}
