#include <sys/vfs.h>
#include <sys/errors.h>
#include <mc/string.h>
#include <sys/uart.h>
#include <sys/heap.h>
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
        //uart_print(u"--- '%s' against '%s' matches? %s\n", tmp->mntto, path, ((match) ? u"yes" : u"no"));
        if (len >= s && match && path[s] == '/' && s > bests)
        {
            bests = s;
            bestm = tmp;
        }
        tmp = tmp->next;
    }

    if (bestm == NULL) return ENOENT;
    uart_print(u"[vfs_lookup] Best mount is %s\n", bestm->mntto);
    *rest = path + bests;
    *mp = bestm;
    return EOK;
}


int vfs_open( const char16_t *path, uint32_t flags, struct file **fp )
{
    if (path == NULL || path[0] == 0) return EINVALID;

    // get the corresponding mount point
    struct mount *mp;
    const char16_t *name;
    int result = vfs_lookup(path, &mp, &name);
    if (result < 0) return result;

    // create file pointer
    struct file *tmp = heap_allocate( sizeof(struct file) + strlen(path) + 1 );
    if (tmp == NULL) return EMEMORY;
    memset(tmp, 0, sizeof(tmp));
    tmp->mp = mp;
    tmp->path = (char16_t*) ((uint8_t*) tmp + sizeof(*tmp));
    strcpy(tmp->path, path);

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
    if (fp == NULL || buffer == NULL) return EINVALID;
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
