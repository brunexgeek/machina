#include "simple.h"
#include <sys/errors.h>
#include <sys/log.h>
#include <sys/vfs.h>
#include <sys/heap.h>
#include <mc/stdlib.h>
#include <mc/string.h>

#define ESCAPE_WITH_ERROR(x) do {result = (x); goto ESCAPE; } while (false)

#define SIMPLE_SIGNATURE 0x11223344
/*

Disk layout

+-----------------------------+
| Header (512 bytes + padding |
| to fit a cluster)           |
+-----------------------------+
| FAT (16 or 32 bits entry)   |
+-----------------------------+
| Data area                   |
+-----------------------------+

The size of the FAT depends on the amount of clusters available. The amount
of clusters is given by dividing the total space of the device by the cluster
size. If the number of clusters is greater than 2^16, the FAT entries will have
32 bits. Otherwise, the FAT entries will have 16 bits.

*/


struct header_t
{
    uint32_t jump;
    uint32_t signature;
    uint16_t version;
    /**
     * Sector size.
     *
     * The value in this field is multiplied by 512 to get the
     * effective sector size.
     */
    uint8_t sector_size;
    /**
     * Number of sectors each cluster have.
     */
    uint8_t cluster_size;
    /**
     * Number of clusters in the partition.
     */
    uint32_t clusters;
    /**
     * Number of the first cluster for the file allocation table.
     */
    uint32_t fat_start;
    /**
     * Number of clusters necessary to store the file allocation table .
     */
    uint32_t fat_size;
    /**
     * Number of the first cluster for the root directory.
     */
    uint32_t root_start;
    /**
     * Partition serial number.
     */
    uint8_t serial[8];
    /**
     * Partition name as UTF-8 string.
     *
     * All bytes can be used to store the name. Unused bytes must be zero.
     */
    uint8_t name[16];
    /**
     * Number of bits for cluster indices.
     *
     * Must be 16 or 32. This value defines the size of each FAT entry.
     */
    uint8_t index_size;
    /**
     * Operating system specific data.
     */
    uint8_t data[457];
    /**
     * Header marker (0x55 0xAA).
     */
    uint16_t marker;
};

static_assert(sizeof(header_t) == 512, "'header_t' must be 512 bytes long");

enum file_flag_t
{
    FLAG_USR_READ   = 0x0001,
    FLAG_USR_WRITE  = 0x0002,
    FLAG_USR_EXEC   = 0x0004,
    FLAG_GRP_READ   = 0x0008,
    FLAG_GRP_WRITE  = 0x0010,
    FLAG_GRP_EXEC   = 0x0020,
    FLAG_OTH_READ   = 0x0040,
    FLAG_OTH_WRITE  = 0x0080,
    FLAG_OTH_EXEC   = 0x0100,
    FLAG_SYSTEM     = 0x0200,
    FLAG_DIRECTORY  = 0x0400,
    FLAG_ARCHIVE    = 0x0800,
};

#define INDEX_UNUSED  0x00000000
#define INDEX_EOC     0x0FFFFFFE
#define INDEX_BAD     0x0FFFFFFF

#define INDEX_MASK(x) ((x) & 0x0FFFFFFF)

#define INODE_UNUSED    (char) 0x81
#define INODE_EOC       (char) 0x82

struct inode_t
{
    /**
     * Index for the first cluster of the file.
     */
    uint32_t cluster;
    /**
     * Permissions and attributes  (see 'file_flag_t').
     */
    uint16_t flags;
    /**
     * UTF-8 string representing the file name.
     *
     * If the entry is unused, the first byte must be 0x81. If the entry is unused and
     * there's no other valid entry after, the first byte must be 0x82.
     */
    char name[46];
    /**
     * 16-bits hash of the file name.
     *
     * Used to optimize searches.
     */
    uint16_t hash;
    uint8_t owner;
    uint8_t group;
    /**
     * File size in bytes.
     */
    uint32_t size;
    /**
     * Modification date and time.
     */
    uint32_t date;
};

static_assert(sizeof(inode_t) == 64, "'inode_t' must be 64 bytes long");

struct file_name_t
{
    /**
     * Only used for FLAG_LONGNAME if next entry is part of the file name.
     */
    uint16_t flags;
    /**
     * UTF-8 string representing abother part of the file name.
     *
     * This field holds the 30 bytes of the file name. If the file name
     * is even longer, the flag FLAG_LONGNAME must be set and the next entry
     * must be a 'file_name_t' with the following characters. Unused bytes must be zero.
     *
     * If the entry is unused, the first byte must be 0x81. If the entry is unused and
     * there's no other valid entry after, the first byte must be 0x82.
     **/
    char name[30];
};

#define SECTOR_SIZE  512
#define CLUSTER_SIZE  4096
#define SECTOR_PER_CLUSTER CLUSTER_SIZE / SECTOR_SIZE

struct internal_t
{
    // TODO: include lock for multi-threaded partition access
    uint8_t *buffer;
    header_t header;
    uint32_t *fat;
};

static int read_sector( device_t *dev, size_t sector, void *buffer, size_t count )
{
    return dev->driver->dev_api.storage.read(dev, sector, buffer, count);
}

static int write_sector( device_t *dev, size_t sector, void *buffer, size_t count )
{
    return dev->driver->dev_api.storage.write(dev, sector, buffer, count);
}

static int read_cluster( device_t *dev, size_t cluster, void *buffer, size_t count )
{
    return dev->driver->dev_api.storage.read(dev, cluster / 512, buffer, count);
}

static int write_cluster( device_t *dev, size_t cluster, void *buffer, size_t count )
{
    return dev->driver->dev_api.storage.write(dev, cluster / 512, buffer, count);
}

uint32_t fat_next_cluster( mount_t *mp, uint32_t cluster )
{
    auto internal = (const internal_t*) mp->fsdata;
    if (cluster >= internal->header.clusters) return INDEX_BAD;
    return internal->fat[cluster];
}

/**
 * Adapted version of Bob Jenkins' "one at a time" 32 bits hash function.
 */
static uint32_t hash32( const char *value, int length )
{
    uint32_t hash = 0;
    while (*value != 0 && length > 0)
    {
        hash += (uint32_t) *value++;
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    return hash + (hash << 15);
}

static uint16_t hash16( const char *value, int length )
{
    uint32_t hash = hash32(value, length);
    return (uint16_t) ( (hash >> 16) ^ (hash & 0xFFFF) );
}

/**
 * Lookup for the inode corresponding to the given path.
 *
 * This function performs a recursive search for the inode represented by
 * an absolute path.
 *
 * @param cluster First cluster of the directory inode. Must be the root directory
 *     in the first call.
 */
static int inode_lookup( mount_t *mp, uint32_t cluster, const char *path, inode_t *inode )
{
    if (mp == nullptr || path == nullptr || inode == nullptr)
        return EARGUMENT;

    auto internal = (const internal_t*) mp->fsdata;
    auto buffer = internal->buffer;

    // compute the length of the next component
    while (*path == '/') ++path;

    int max_entries = (internal->header.cluster_size * 512) / sizeof(inode_t);
    int result = read_cluster(mp->dev, cluster, internal->buffer, 1);
    if (result) return result;

    while (*path != 0)
    {
        inode_t *ptr;

        // compute the length of the current path component
        size_t i = 0;
        while (*path != '/' && *path != 0) ++i;
        if (i == 0) break;

        uint16_t hash = hash16(path, i);

        for (int i = 0; i < max_entries; ++i)
        {
            ptr = (inode_t*) buffer + i;
            if (hash == ptr->hash && strncmp(path, ptr->name, i) == 0)
            {
                klog_print("  -- Found\n");

                // we have more components to find?
                path += i;
                while (*path == '/') ++path;
                if (*path == 0)
                {
                    memcpy(inode, ptr, sizeof(inode_t));
                    return EOK;
                }
                else
                {
                    // to continue the lookup the current inode must be a directory
                    if (!(ptr->flags & FLAG_DIRECTORY)) return EINVALID;
                    // follow the current inode
                    return inode_lookup(mp, ptr->cluster, path, inode);
                }
            }
        }

        // check whether the directory have more entries
        cluster = INDEX_MASK( fat_next_cluster(mp, cluster) );
        if (cluster == INDEX_BAD || cluster == INDEX_EOC)
            return ENOENT;
        // read the next cluster of the directory
        result = read_cluster(mp->dev, cluster, internal->buffer, 1);
        if (result) return result;
    }

    return ENOENT;
}

static int simple_open( file_t *fp, const char *path, uint32_t flags )
{
    auto internal = (const internal_t*) fp->mp->fsdata;
    inode_t inode;
    inode_lookup(fp->mp, internal->header.root_start, path, &inode);
    return ENOIMP;
}

static int simple_close( file_t *fp )
{
    return ENOIMP;
}

static int simple_read( file_t *fp, uint8_t *buffer, size_t count )
{
    return ENOIMP;
}

static int simple_write( file_t *fp, const uint8_t *buffer, size_t count )
{
    (void) fp;
    (void) buffer;
    (void) count;
    return ENOIMP;
}

static int simple_stat( file_t *fp, struct stat *info )
{
    (void) fp;
    (void) info;
    return ENOIMP;
}

static int simple_enumerate( file_t *fp, struct dirent *entry )
{
    (void) fp;
    (void) entry;
    return ENOIMP;
}

static int simple_mkfs( device_t *dev, const char *opts )
{
    size_t size = 0;
    int result = dev->driver->dev_api.storage.size(dev, &size);
    if (result != EOK) return result;
    size = size & (~CLUSTER_SIZE);
    size_t clusters = size / CLUSTER_SIZE;

    if (clusters > 65530) return ETOOLONG;

    uint8_t tmp[512];
    memset(tmp, 0, sizeof(tmp));

    header_t *header = (header_t*) tmp;
    header->sector_size = 1; // 512
    header->cluster_size = 8; // 4096 KB
    header->clusters = clusters;
    header->fat_start = 1;
    header->fat_size = clusters * 4 / header->cluster_size + header->cluster_size;
    header->root_start = header->fat_start + header->fat_size;
    //header->serial
    //header->name
    header->index_size = (clusters > 0x010000) ? 32 : 16;
    header->marker = 0xAA55;
    write_sector(dev, 0, header, 1);

    // zero out FAT entries
    memset(tmp, 0, sizeof(tmp));
    for (int i = 0; i < header->fat_size * 8; ++i)
        write_sector(dev, 4 + i, tmp, 512);

    // some dark magic
    memset(tmp, 0, sizeof(tmp));
    inode_t *inode = (inode_t*) tmp;
    // create and empty root directory
    inode->cluster = INDEX_EOC;
    inode->name[0] = INODE_EOC;
    write_sector(dev, 0, inode, 4);

    return EOK;
}

static int simple_mount( mount_t *mp, const char *opts, uint32_t flags )
{
    (void) mp;
    (void) opts;
    (void) flags;
    int result;

    mp->fsdata = heap_allocate(sizeof(internal_t));
    if (mp->fsdata == nullptr) return EMEMORY;
    internal_t *intr = (internal_t*) mp->fsdata;

    // read the header
    read_cluster(mp->dev, 0, intr->buffer, 1);
    memcpy(&intr->header, intr->buffer, sizeof(internal_t));
    if (intr->header.signature != SIMPLE_SIGNATURE)
        ESCAPE_WITH_ERROR(EINVALID);
    // read the FAT entries
    intr->fat = (uint32_t*) heap_allocate(intr->header.fat_size * 4096);
    if (intr->fat == nullptr)
        ESCAPE_WITH_ERROR(EMEMORY);
    read_cluster(mp->dev, intr->header.fat_start, intr->fat, intr->header.fat_size * 4096);

    return EOK;

ESCAPE:
    if (intr)
    {
        if (intr->fat) heap_free(intr->fat);
        heap_free(intr);
    }
    return result;
}

static int simple_unmount( mount_t *mp )
{
    (void) mp;
    return EOK;
}

int simple_initialize()
{
    static filesystem_t fs;
    strcpy(fs.type, "simplefs");
    fs.ops.open = simple_open;
    fs.ops.close = simple_close;
    fs.ops.read = simple_read;
    fs.ops.write = simple_write;
    fs.ops.stat = simple_stat;
    fs.ops.enumerate = simple_enumerate;
    fs.ops.mount = simple_mount;
    fs.ops.unmount = simple_unmount;
    fs.ops.mkfs = simple_mkfs;

    return vfs_register(&fs);
}