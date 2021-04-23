#include <sys/vfs.h>
#include <sys/errors.h>
#include <mc/string.h>
#include <sys/heap.h>
#include <mc/string.h>


#define MAX_FS_ENTRIES 8


filesystem_t *fsList;

mount_t *mountList;


int vfs_initialize()
{
    fsList = NULL;
    mountList = NULL;
    return 0;
}


int vfs_register( filesystem_t *fs )
{
    bool valid =
        fs != NULL &&
        fs->type[0] != 0 &&
        fs->ops.open != NULL &&
        fs->ops.close != NULL &&
        fs->ops.read != NULL &&
        fs->ops.write != NULL &&
        fs->ops.enumerate != NULL &&
        fs->ops.mount != NULL &&
        fs->ops.unmount != NULL;
    if (!valid) return EINVALID;

    filesystem_t *p = fsList;
    while (p)
    {
        if (strcmp(p->type, fs->type) == 0) return EEXIST;
        p = p->next;
    }

    fs->next = fsList;
    fsList = fs;

    //uart_print("Registered filesystem '%s'\n", fs->type);

    return EOK;
}


int vfs_unregister( const char *type )
{
    if (type == NULL || type[0] == 0) return EINVALID;

    filesystem_t *q = NULL;
    filesystem_t *p = fsList;
    while (p)
    {
        if (strcmp(p->type, type) == 0)
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


int vfs_mount( const char *type, device_t *dev, const char *target, const char *opts,
    uint32_t flags, mount_t **mp )
{
    if (strlen(target) >= MAX_PATH) return ETOOLONG;

    filesystem_t *fs = fsList;
    while (fs)
    {
        if (strcmp(fs->type, type) == 0) break;
        fs = fs->next;
    }
    if (fs == NULL) return ENOENT;

    mount_t *tmp = (mount_t*) heap_allocate(sizeof(mount_t));
    if (tmp == NULL) return EMEMORY;

    memset(tmp, 0, sizeof(*tmp));
    strcpy(tmp->target, target);
    tmp->fs = fs;

    int result = fs->ops.mount(tmp, opts, flags);
    if (result < 0)
    {
        heap_free(tmp);
        return result;
    }

    tmp->next = mountList;
    mountList = tmp;
    if (mp) *mp = tmp;
    return EOK;
}


int vfs_unmount( const char *target, uint32_t flags )
{
    (void) flags;

    if (target == NULL) return EINVALID;

    mount_t *p = NULL;
    mount_t *m = mountList;

    while (strcmp(m->target, target) == 0)
    {
        p = m;
        m = m->next;
    }
    if (m == NULL) return ENOENT;

    int result =  m->fs->ops.unmount(m);
    if (result < 0) return result;

    if (m == mountList)
        mountList = m->next;
    else
        p->next = m->next;

    heap_free(m);
    return EOK;
}


int vfs_lookup( const char *path, mount_t **mp, const char **rest)
{
    if (!path) return EINVALID;

    size_t len = strlen(path);
    mount_t *bestm = NULL;
    size_t bests = 0;

    // find matching mount point
    mount_t *tmp = mountList;
    while (tmp)
    {
        size_t s = strlen(tmp->target);
        bool match = (strncmp(tmp->target, path, s) == 0);
        if (len >= s && match && path[s] == '/' && s > bests)
        {
            bests = s;
            bestm = tmp;
        }
        tmp = tmp->next;
    }

    if (bestm == NULL) return ENOENT;
    *rest = path + bests;
    *mp = bestm;
    return EOK;
}


int vfs_open( const char *path, uint32_t flags, file_t **fp )
{
    if (path == NULL || path[0] == 0) return EINVALID;

    // get the corresponding mount point
    mount_t *mp;
    const char *name;
    int result = vfs_lookup(path, &mp, &name);
    if (result < 0) return result;

    // create file pointer
    file_t *tmp = (file_t *) heap_allocate( sizeof(file_t) + strlen(path) + 1 );
    if (tmp == NULL) return EMEMORY;
    memset(tmp, 0, sizeof(tmp));
    tmp->mp = mp;
    tmp->path = (char*) ((uint8_t*) tmp + sizeof(*tmp));
    strcpy(tmp->path, path);

    result = mp->fs->ops.open(tmp, name, flags);
    if (result < 0)
    {
        heap_free(tmp);
        return result;
    }

    *fp = tmp;
    return result;
}

int vfs_close( file_t *fp )
{
    if (fp == NULL) return EINVALID;
    return fp->mp->fs->ops.close(fp);
}

int vfs_read( file_t *fp, uint8_t *buffer, size_t count )
{
    if (fp == NULL || buffer == NULL) return EINVALID;
    return fp->mp->fs->ops.read(fp, buffer, count);
}

int vfs_write( file_t *fp, const uint8_t *buffer, size_t count )
{
    if (fp == NULL) return EINVALID;
    return fp->mp->fs->ops.write(fp, buffer, count);
}

int vfs_enumerate( file_t *fp, struct dirent *entry )
{
    if (fp == NULL) return EINVALID;
    return fp->mp->fs->ops.enumerate(fp, entry);
}
