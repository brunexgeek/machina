#include <sys/procfs.h>
#include <sys/vfs.h>
#include <sys/errors.h>
#include <mc/string.h>
#include <sys/heap.h>
#include <sys/uart.h>
#include <mc/stdlib.h>
#include <mc/memory.h>


#define PROCFS_MAX_BUFFER   4096

struct inode
{
    char16_t name[MAX_FILENAME];
    ino_t inode;
    procfunc_t callback;
    void *data;
    struct inode *next;
};

struct fsdata
{
    struct inode *inode;
    uint8_t buffer[PROCFS_MAX_BUFFER]; // TODO: make this dynamic
    int offset;
    int size;
};

static struct inode *procList = NULL;

static ino_t counter = 0;


static struct inode *find_inode( const char16_t *name )
{
    struct inode *node = procList;
    while (node)
    {
        if (strcmp(name, node->name) == 0) return node;
        node = node->next;
    }

    return NULL;
}


static struct inode *remove_inode( const char16_t *name )
{
    struct inode *prev = NULL;
    struct inode *node = procList;
    while (node)
    {
        if (strcmp(name, node->name) == 0)
        {
            if (prev)
                prev->next = node->next;
            else
                procList->next = node->next;
            node->next = NULL;
            return node;
        }
        prev = node;
        node = node->next;
    }

    return NULL;
}


static int procfs_open( struct file *fp, const char16_t *path, uint32_t flags )
{
    (void) flags;

    struct inode *inode = find_inode(path);
    if (inode == NULL) return ENOENT;

    // create procfs specific data
    struct fsdata *data = (struct fsdata*) heap_allocate( sizeof(struct fsdata) );
    if (data == NULL) return EMEMORY;
    memset(data, 0, sizeof(*data));
    data->inode = inode;

    // call registered function with a fixed size internal buffer
    int result = inode->callback(data->buffer, PROCFS_MAX_BUFFER, inode->data);
    if (result < 0 || result > PROCFS_MAX_BUFFER)
    {
        heap_free(data);
        return (result < 0) ? result : ETOOLONG;
    }

    data->size = result;
    fp->fsdata = data;
    return EOK;
}

static int procfs_close( struct file *fp )
{
    if (fp->fsdata) heap_free(fp->fsdata);
    return EOK;
}

static int procfs_read( struct file *fp, uint8_t *buffer, size_t count )
{
    if (count == 0) return 0;

    struct fsdata *pd = (struct fsdata*)fp->fsdata;
    if (pd->size == pd->offset) return 0;

    int len = min((int) count, pd->size - pd->offset);
    memcpy(buffer, pd->buffer + pd->offset, (size_t) len);
    pd->offset += len;

    return len;
}

static int procfs_write( struct file *fp, const uint8_t *buffer, size_t count )
{
    (void) fp;
    (void) buffer;
    (void) count;
    return ENOIMP;
}

static int procfs_stat( struct file *fp, struct stat *info )
{
    (void) fp;
    (void) info;
    return ENOIMP;
}

static int procfs_enumerate( struct file *fp, struct dirent *entry )
{
    (void) fp;
    (void) entry;
    return ENOIMP;
}

static int procfs_mount( struct mount *mp, const char16_t *opts, uint32_t flags )
{
    (void) mp;
    (void) opts;
    return EOK;
}

static int procfs_unmount( struct mount *mp )
{
    (void) mp;
    return EOK;
}


int procfs_initialize()
{
    static struct filesystem fs;
    strcpy(fs.type, u"procfs");
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

    if (find_inode(name)) return EEXIST;

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

int procfs_unregister( const char16_t *name )
{
    struct inode *tmp = remove_inode(name);
    if (tmp == NULL) return ENOENT;

    heap_free(tmp);
    return EOK;
}
