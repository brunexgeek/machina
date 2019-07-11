#ifndef MACHINA_VFS_H
#define MACHINA_VFS_H


#include <sys/types.h>


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


struct dirent
{
    char16_t name[MAX_FILENAME + 1]; /* null-terminated file name */
    ino_t inode;
    uint32_t size;
    uint16_t type;
    uint16_t flags;
};


struct stat
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

struct filesystem;

struct mount
{
    char16_t source[MAX_PATH];
    char16_t target[MAX_PATH];
    struct filesystem *fs;
    uid_t uid;
    gid_t gid;
    uint32_t mode;
    void *fsdata; // fylesystem-specific data
    struct mount *next;
};

struct file
{
    struct mount *mp;
    uint32_t inode;
    int flags;
    int mode;
    uid_t owner;
    gid_t group;
    off64_t offset;
    char16_t *path;
    char chbuf;
    void *fsdata; // fylesystem-specific data
};


struct fsops
{
    /**
     * Open the file pointed by the directory entry.
     *
     * @returns On success, returns a positive file descriptor. Otherwise,
     *     returns a negative error code.
     */
    int (*open)( struct file *fp, const char16_t *path, uint32_t flags );

    /**
     * Close a file description.
     *
     * @returns On success, returns zero. Otherwise, returns a negative error code.
     */
    int (*close)( struct file *fp );

    /**
     * Attempts to read up to @c count bytes from file descriptor @c fd into @c buffer.
     *
     * @returns On success, returns the number of bytes read (zero means end of file),
     *     and the file position is advanced by this number. The number of read bytes can
     *     be smaller than @c count. Otherwise, returns a negative error code.
     */
    int (*read)( struct file *fp, uint8_t *buffer, size_t count );

    /**
     * Writes up to @c count bytes from @c buffer to the file descriptor @c fd.
     *
     * @returns On success, returns the number of bytes written (zero means nothing was written).
     *     The number of written bytes can be smaller than @c count. Otherwise, returns a
     *     negative error code.
     */
    int (*write)( struct file *fp, const uint8_t *buffer, size_t count );

    int (*stat)( struct file *fp, struct stat *info );

    /**
     * Reads to @c entry the next directory entry from the file descriptor @c fd.
     *
     * The file descriptor @c fd must point to a directory (opened with @c open).
     *
     * @returns On success, the next directory entry information is copied to @c entry and
     *     the function returns zero. Otherwise, returns a negative error code.
     */
    int (*enumerate)( struct file *fp, struct dirent *entry );

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
    //int (*find)( struct mount *mp, const char16_t *name, struct vfs_noderef *ref );

    int (*mount)( struct mount *mp, const char16_t *opts, uint32_t flags );

    int (*unmount)( struct mount *mp );

    // TODO: 'remove', 'seek', 'tell'
};

struct filesystem
{
    char16_t type[MAX_FILENAME + 1]; /* null-terminated file system name */
    struct fsops ops;
    struct filesystem *next;
};


#ifdef __cplusplus
extern "C" {
#endif

int vfs_initialize();

int vfs_register( struct filesystem *fs );

int vfs_unregister( const char16_t *type );

int vfs_mount(
    const char16_t *type,
    const char16_t *source,
    const char16_t *target,
    const char16_t *opts,
    uint32_t flags,
    struct mount **mp );

int vfs_unmount( const char16_t *target, uint32_t flags );

int vfs_lookup( const char16_t *name, struct mount **mp, const char16_t **rest);

int vfs_open( const char16_t *name, uint32_t flags, struct file **fp );

int vfs_close( struct file *fp );

int vfs_read( struct file *fp, uint8_t *buffer, size_t count );

int vfs_write( struct file *fp, const uint8_t *buffer, size_t count );

int vfs_enumerate( struct file *fp, struct dirent *entry );


// default implementation for 'fs->find' (because most of the time file systems will do the same thing)
//int vfs_defaultFind( struct vfs_mount *mp, const char16_t *name, struct vfs_noderef *ref );

#ifdef __cplusplus
}
#endif

#endif // MACHINA_VFS_H
