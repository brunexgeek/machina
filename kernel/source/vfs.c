#include <sys/vfs.h>
#include <sys/errors.h>
#include <mc/string.h>
#include <sys/uart.h>
#include <sys/heap.hh>
#include <mc/memory.h>


#define MAX_FS_ENTRIES 8


struct filesystem *fsList;

struct mount *mountList;


int vfs_initialize()
{
    fsList = NULL;
    mountList = NULL;
    return 0;
}

#if 0
static int vfs_allocfd( struct mount *mp, ino_t inode )
{
    if (mp == NULL) return EINVALID;

    for (int i = 0; i < MAX_FD; ++i)
    {
        if (fdTable[i].mp == NULL)
        {
            fdTable[i].inode = inode;
            fdTable[i].mp = mp;
            return i;
        }
    }
    return EEXHAUSTED;
}


static int vfs_freefd( int fd )
{
    if (!IS_VALID_FD(fd)) return EINVALID;

    fdTable[fd].inode = 0;
    fdTable[fd].mp = NULL;
    return EOK;
}
#endif

int vfs_register( struct filesystem *fs )
{
    bool valid =
        fs != NULL &&
        fs->name[0] != 0 &&
        fs->ops.open != NULL &&
        fs->ops.close != NULL &&
        fs->ops.read != NULL &&
        fs->ops.write != NULL &&
        fs->ops.enumerate != NULL &&
        fs->ops.mount != NULL &&
        fs->ops.unmount != NULL;
    if (!valid) return EINVALID;

    struct filesystem *p = fsList;
    while (p)
    {
        if (strcmp(p->name, fs->name) == 0) return EEXIST;
        p = p->next;
    }

    fs->next = fsList;
    fsList = fs;

    uart_print(u"Registered filesystem '%s'\n", fs->name);

    return EOK;
}

int vfs_unregister( const char16_t *name )
{
    if (name == NULL || name[0] == 0) return EINVALID;

    struct filesystem *q = NULL;
    struct filesystem *p = fsList;
    while (p)
    {
        if (strcmp(p->name, name) == 0)
        {
            if (q)
                q->next = p->next;
            else
                fsList = p->next;
            return EOK;
        }
        p = p->next;
    }

    return ENOENT;
}


int vfs_mount(
    const char16_t *type,
    const char16_t *mntfrom,
    const char16_t *mntto,
    const char16_t *opts,
    struct mount **mp )
{
    if (strlen(mntto) >= MAX_PATH || strlen(mntfrom) >= MAX_PATH) return ETOOLONG;

    struct filesystem *fs = fsList;
    while (fs)
    {
        if (strcmp(fs->name, type) == 0) break;
        fs = fs->next;
    }
    if (fs == NULL) return ENOENT;

    struct mount *tmp = (struct mount*) heap_allocate(sizeof(struct mount));
    if (tmp == NULL) return EMEMORY;

    memset(tmp, 0, sizeof(*tmp));
    strcpy(tmp->mntto, mntto);
    strcpy(tmp->mntfrom, mntfrom);
    tmp->fs = fs;

    int result = fs->ops.mount(tmp, opts);
    if (result < 0)
    {
        heap_free(tmp);
        return result;
    }

    tmp->next = mountList;
    mountList = tmp;
    return EOK;
}

int vfs_unmount( struct mount *mp )
{
    if (mp == NULL) return EINVALID;
    int result =  mp->fs->ops.unmount(mp);
    if (result < 0) return result;

    if (mp == mountList)
        mountList = mp->next;
    else
    {
        struct mount *q = mountList;
        while (q)
        {
            if (q->next == mp)
            {
                q->next = mp->next;
                break;
            }
            q = q->next;
        }
    }

    heap_free(mp);
    return EOK;
}

/*
static int vfs_find( struct mount *mp, const char16_t *name, struct vfs_noderef *ref )
{
    if (mp == NULL) return EINVALID;
    return mp->fs->ops.find(mp, name, ref);
}*/


int vfs_lookup( const char16_t *path, struct mount **mp, const char16_t **rest)
{
    if (!path) return EINVALID;

    size_t len = strlen(path);
    struct mount *bestm = NULL;
    size_t bests = 0;

    // find matching mount point
    struct mount *tmp = mountList;
    while (tmp)
    {
        size_t s = strlen(tmp->mntto);
        bool match = (strncmp(tmp->mntto, path, s) == 0);
        uart_print(u"--- '%s' against '%s' matches? %s\n", tmp->mntto, path, ((match) ? u"yes" : u"no"));
        if (len >= s && match && path[s] == '/' && s > bests)
        {
            bests = s;
            bestm = tmp;
        }
        tmp = tmp->next;
    }

    if (bestm == NULL) return ENOENT;
    uart_print(u"Best mount is %s\n", bestm->mntto);
    *rest = path + bests;
    *mp = bestm;
    return EOK;
}


int vfs_open( const char16_t *path, uint32_t flags, struct file **fp )
{
    if (path == NULL || path[0] == 0) return EINVALID;

    struct mount *mp;
    const char16_t *name;
    int result = vfs_lookup(path, &mp, &name);
    if (result < 0) return result;

    struct file *tmp = heap_allocate( sizeof(struct file) );
    if (tmp == NULL) return EMEMORY;
    FillMemory(tmp, 0, sizeof(tmp));
    tmp->mp = mp;
    // TODO: set 'tmp->path' with name

    result = mp->fs->ops.open(tmp, name, flags);
    if (result < 0)
    {
        heap_free(tmp);
        return result;
    }

    *fp = tmp;
    return EOK;
}

int vfs_close( struct file *fp )
{
    if (fp == NULL) return EINVALID;
    return fp->mp->fs->ops.close(fp);
}

int vfs_read( struct file *fp, uint8_t *buffer, size_t count )
{
    if (fp == NULL) return EINVALID;
    return fp->mp->fs->ops.read(fp, buffer, count);
}

int vfs_write( struct file *fp, const uint8_t *buffer, size_t count )
{
    if (fp == NULL) return EINVALID;
    return fp->mp->fs->ops.write(fp, buffer, count);
}

int vfs_enumerate( struct file *fp, struct dirent *entry )
{
    if (fp == NULL) return EINVALID;
    return fp->mp->fs->ops.enumerate(fp, entry);
}

#if 0
static int path_next( const char16_t *path, char16_t *current, const char16_t **next )
{
    if (path == NULL) return EINVALID;
    while (*path == '/') ++path;

    const char16_t *ptr = path;
    current[0] = 0;

    while (*ptr != '/' && *ptr != 0) ++ptr;
    size_t len = (size_t) (ptr - path);

    if (*ptr == '/')
    {
        while (*ptr == '/') ++ptr;
        *next = ptr;
    }
    else
        *next = NULL;

    if (len == 0)
        return ENODATA;
    if (len > MAX_FILENAME)
        return ETOOLONG;

    strncpy(current, path, len);
    current[len] = 0;
    return EOK;
}


int vfs_defaultFind( struct mount *mp, const char16_t *name, struct vfs_noderef *ref )
{
    if (mp == NULL || name == NULL || name[0] == 0 || ref == NULL)
        return EINVALID;

    uint32_t cn = mp->root;
    struct vfs_dirent info;
    const char16_t *ptr = name;
    char16_t current[MAX_FILENAME + 1];
    int fd;
    int result;

    while (*ptr != 0)
    {
RESET:
        // copy the current file name (until the next back-slash)
        result = path_next(ptr, current, &ptr);
        if (result < 0) return result;
        // open the current inode
        fd = mp->fs->ops.open(mp, cn, 0);
        if (fd < 0) return fd;
        // look for this file in the current entry
        while (mp->fs->ops.enumerate(mp, fd, &info) == EOK)
        {
            uart_print(u"-- %s\n", current);
            if (strcmp(info.name, current) != 0) continue;

            mp->fs->ops.close(mp, fd);

            // check if current is the last part in the path
                // if TRUE, copy 'tmp' address to 'entry' and return 0
                // if FALSE
                    // check if 'tmp' is a directory
                        // if FALSE, return error
                        // if TRUE
                            // check if 'tmp' is a mount point
                                // if TRUE, call 'tmp->mp->fs->ops.find()' and return its return value
                                // if FALSE, break the inner loop to keep going

            if (ptr == NULL)
            {
                ref->inode = info.inode;
                ref->mp = mp;
                return EOK;
            }
            if (info.type != VFS_DIRECTORY)
                return EINVALID;
            if (info.flags & VFS_MOUNTPOINT)
                return info.mp->fs->ops.find(info.mp, ptr, ref);
            goto RESET;
        }
        mp->fs->ops.close(mp, fd);
        return ENOENT;
    }

    return ENOENT;
}

#endif