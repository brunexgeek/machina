#include <sys/ramfs.h>
#include <sys/vfs.h>
#include <sys/errors.h>
#include <mc/string.h>

struct entry_t
{
    char *path;
    /**
     * Pointer to the data in memory
     */
    void *ptr;
};

struct internal_t
{

};

static int ramfs_open( file_t *fp, const char *path, uint32_t flags )
{
    return ENOIMP;
}

static int ramfs_close( file_t *fp )
{
    return ENOIMP;
}

static int ramfs_read( file_t *fp, uint8_t *buffer, size_t count )
{
    return ENOIMP;
}

static int ramfs_write( file_t *fp, const uint8_t *buffer, size_t count )
{
    return ENOIMP;
}

static int ramfs_stat( file_t *fp, struct stat *info )
{
    return ENOIMP;
}

static int ramfs_enumerate( file_t *fp, struct dirent *entry )
{
    return ENOIMP;
}

static int ramfs_mount( mount_t *mp, const char *opts, uint32_t flags )
{
    return ENOIMP;
}

static int ramfs_unmount( mount_t *mp )
{
    (void) mp;

    return ENOIMP;
}

int ramfs_initialize()
{
    filesystem_t fs;
    strcpy(fs.type, "ramfs");
    fs.ops.open = ramfs_open;
    fs.ops.close = ramfs_close;
    fs.ops.read = ramfs_read;
    fs.ops.write = ramfs_write;
    fs.ops.stat = ramfs_stat;
    fs.ops.enumerate = ramfs_enumerate;
    fs.ops.mount = ramfs_mount;
    fs.ops.unmount = ramfs_unmount;

    return vfs_register(&fs);
}
