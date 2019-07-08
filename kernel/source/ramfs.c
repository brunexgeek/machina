#if 0

#include <sys/ramfs.h>
#include <sys/vfs.h>
#include <sys/errors.h>
#include <mc/string.h>

int ramfs_open( struct file *fp, const char16_t *path, uint32_t flags )
{
    return ENOIMP;
}

int ramfs_close( struct file *fp )
{
    return ENOIMP;
}

int ramfs_read( struct file *fp, uint8_t *buffer, size_t count )
{
    return ENOIMP;
}

int ramfs_write( struct file *fp, const uint8_t *buffer, size_t count )
{
    return ENOIMP;
}

int ramfs_stat( struct file *fp, struct stat *info )
{
    return ENOIMP;
}

int ramfs_enumerate( struct file *fp, struct dirent *entry )
{
    return ENOIMP;
}

int ramfs_mount( struct mount *mp, const char16_t *opts )
{
    return ENOIMP;
}

int ramfs_unmount( struct mount *mp )
{
    (void) mp;

    return ENOIMP;
}


int ramfs_initialize()
{
    static struct filesystem fs;
    strcpy(fs.name, u"ramfs");
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

#endif
