#ifndef MACHINA_VFS_H
#define MACHINA_VFS_H

#include <sys/types.h>
#include <sys/device.hh>

#define MAX_FILENAME       63
#define MAX_PATH           256 // including null-terminator
#define MAX_FSNAME         15
#define MAX_FD             256 // maximum number of file descriptors
#define IS_VALID_FD(x)     (fd >= 0 && fd < MAX_FD)

/* inode.type */
#define VFS_FILE           0x01
#define VFS_DIRECTORY      0x02
#define VFS_SYMLINK        0x03

/* inode.flags */
#define VFS_MOUNTPOINT     0x01


struct dirent_t
{
    char name[MAX_FILENAME + 1]; /* null-terminated file name */
    ino_t inode;
    uint32_t size;
    uint16_t type;
    uint16_t flags;
};


struct stat_t
{
    ino_t inode;
    uint64_t size;
    uint16_t type;
    uint16_t flags;
    uint32_t mode; // permissions
    uid_t uid;
    gid_t gid;
    time_t atime;
    time_t mtime;
    time_t ctime;
};

struct filesystem_t;

struct mount_t
{
    device_t *dev;
    char target[MAX_PATH];
    filesystem_t *fs;
    uid_t uid;
    gid_t gid;
    uint32_t mode;
    void *fsdata; // fylesystem-specific data
    struct mount_t *next;
};

struct file_t
{
    mount_t *mp;
    uint32_t inode;
    int flags;
    int mode;
    uid_t owner;
    gid_t group;
    off64_t offset;
    char *path;
    char chbuf;
    void *fsdata; // fylesystem-specific data
};


struct fsops_t
{
    /**
     * Open the file pointed by the directory entry.
     *
     * @returns On success, returns a positive file descriptor. Otherwise,
     *     returns a negative error code.
     */
    int (*open)( file_t *fp, const char *path, uint32_t flags );

    /**
     * Close a file description.
     *
     * @returns On success, returns zero. Otherwise, returns a negative error code.
     */
    int (*close)( file_t *fp );

    /**
     * Attempts to read up to @c count bytes from file descriptor @c fd into @c buffer.
     *
     * @returns On success, returns the number of bytes read (zero means end of file),
     *     and the file position is advanced by this number. The number of read bytes can
     *     be smaller than @c count. Otherwise, returns a negative error code.
     */
    int (*read)( file_t *fp, uint8_t *buffer, size_t count );

    /**
     * Writes up to @c count bytes from @c buffer to the file descriptor @c fd.
     *
     * @returns On success, returns the number of bytes written (zero means nothing was written).
     *     The number of written bytes can be smaller than @c count. Otherwise, returns a
     *     negative error code.
     */
    int (*write)( file_t *fp, const uint8_t *buffer, size_t count );

    int (*stat)( file_t *fp, struct stat *info );

    /**
     * Reads to @c entry the next directory entry from the file descriptor @c fd.
     *
     * The file descriptor @c fd must point to a directory (opened with @c open).
     *
     * @returns On success, the next directory entry information is copied to @c entry and
     *     the function returns zero. Otherwise, returns a negative error code.
     */
    int (*enumerate)( file_t *fp, struct dirent *entry );

    /**
     * Fills @c entry with the directory entry information about the file at @c name.
     *
     * The argument @c name must be an absolute path. This function is the only way to
     * get the @c dir_entry_t object from a string path. The @c dir_entry_t object
     * can be used to inspect or open the file.
     *
     * The search can cross file system boundaries (i.e. keep looking for the file
     * through other mount points).
     *
     * @returns On success, the directory entry information is copied to @c entry and
     *     the function returns zero. Otherwise, returns a negative error code.
     */
    //int (*find)( mount_t *mp, const char *name, struct vfs_noderef *ref );

    int (*mount)( mount_t *mp, const char *opts, uint32_t flags );

    int (*unmount)( mount_t *mp );

    // TODO: 'remove', 'seek', 'tell'
};

struct filesystem_t
{
    char type[MAX_FILENAME + 1]; /* null-terminated file system name */
    struct fsops_t ops;
    struct filesystem_t *next;
};


#ifdef __cplusplus
extern "C" {
#endif

int vfs_initialize();

int vfs_register( struct filesystem_t *fs );

int vfs_unregister( const char *type );

int vfs_mount( const char *type, device_t *dev, const char *target, const char *opts,
    uint32_t flags, mount_t **mp );

int vfs_unmount( const char *target, uint32_t flags );

int vfs_lookup( const char *name, struct mount_t **mp, const char **rest);

int vfs_open( const char *name, uint32_t flags, struct file_t **fp );

int vfs_close( struct file_t *fp );

int vfs_read( struct file_t *fp, uint8_t *buffer, size_t count );

int vfs_write( struct file_t *fp, const uint8_t *buffer, size_t count );

int vfs_enumerate( struct file_t *fp, struct dirent_t *entry );


// default implementation for 'fs->find' (because most of the time file systems will do the same thing)
//int vfs_defaultFind( struct vfs_mount *mp, const char *name, struct vfs_noderef *ref );

#ifdef __cplusplus
}
#endif

#endif // MACHINA_VFS_H
