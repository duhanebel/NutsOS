#include "fat16.h"
#include "config.h"
#include "disk/disk.h"
#include "disk/stream.h"
#include "error.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "stdutil/string.h"

// Fat 16 uses 2 bytes to represent a cluster
#define NUTSOS_FAT16_FAT_ENTRY_SIZE        0x02
#define NUTSOS_FAT16_SIGNATURE             0x29
#define NUTSOS_FAT16_UNUSED                0x00

// Special values on filename in dir entry
#define NUTSOS_FAT16_UNUSED_ENTRY          0xE5
#define NUTSOS_FAT16_NOMORE_ENTRY          0x00

#define NUTSOS_FAT16_FREE_CLUSTER          0x0000
#define NUTSOS_FAT16_BAD_SECTOR            0xFFF7
#define NUTSOS_FAT16_IS_LAST_SECTOR(x)     ((x) == 0xFFFF || (x) == 0xFFF8)
#define NUTSOS_FAT16_IS_RESERVED_SECTOR(x) ((x) == 0xFFF0 || (x) == 0xFFF5 || (x) == 0xFFF6)

#define NUTOS_FAT16_SIGNATURE              0x29

typedef enum fat_item_type_
{
  fat_type_directory = 0,
  fat_type_file
} fat_item_type;

// Fat directory entry attributes bitmask
#define FAT_FILE_RESERVED     0x80
#define FAT_FILE_READ_ONLY    0x01
#define FAT_FILE_HIDDEN       0x02
#define FAT_FILE_SYSTEM       0x04
#define FAT_FILE_VOLUME_LABEL 0x08
#define FAT_FILE_SUBDIRECTORY 0x10
#define FAT_FILE_ARCHIVED     0x20
#define FAT_FILE_DEVICE       0x40

// Disk mapped extended fat16 header
struct fat_header_extended {
  uint8_t drive_number;
  uint8_t win_nt_bit;
  uint8_t signature;
  uint32_t volume_id;
  uint8_t volume_id_string[11];
  uint8_t system_id_string[8];
} __attribute__((packed));

// Disk mapped main fat16 header
struct fat_header_main {
  uint8_t short_jmp_ins[3];
  uint8_t oem_identifier[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t fat_copies;
  uint16_t root_dir_entries;
  uint16_t number_of_sectors;
  uint8_t media_type;
  uint16_t sectors_per_fat;
  uint16_t sectors_per_track;
  uint16_t number_of_heads;
  uint32_t hidden_setors;
  uint32_t sectors_big;
} __attribute__((packed));

// Disk mapped fat16 main+extended header struct
struct fat_header {
  struct fat_header_main primary_header;
  union fat_h_e {
    struct fat_header_extended extended_header;
  } shared;
} __attribute__((packed));

// Disk mapped fat16 item structure - can be dir or file
struct fat_directory_item {
  uint8_t filename[8];
  uint8_t ext[3];
  uint8_t attribute;
  uint8_t reserved;
  uint8_t creation_time_tenths_of_a_sec;
  uint16_t creation_time;
  uint16_t creation_date;
  uint16_t last_access;
  uint16_t high_16_bits_first_cluster_or_accessmask;
  uint16_t last_mod_time;
  uint16_t last_mod_date;
  uint16_t low_16_bits_first_cluster;
  uint32_t filesize;
} __attribute__((packed));

// Local structure representing a fat directory
struct fat_directory {
  struct fat_directory_item *item;
  int total;
  int sector_pos;
  int ending_sector_pos;
};

// Local structure representing either a dir or a file
struct fat_item {
  union {
    struct fat_directory_item *item;
    struct fat_directory *directory;
  };

  fat_item_type type;
};

// Local structure representing a fat file descriptor
struct fat_file_descriptor {
  struct fat_item *item;
  uint32_t pos;
};

// Private fat16 driver's fs data
struct fat_private {
  struct fat_header header;
  struct fat_directory root_directory;

  // Convenience
  uint32_t first_fat_sector;
  uint32_t fat_table_address;
  int bytes_per_cluster;

  // Used to stream data clusters
  struct disk_stream *cluster_read_stream;

  // Used to stream the file allocation table
  struct disk_stream *fat_read_stream;

  // Used to stream directory data
  struct disk_stream *directory_stream;
};

int fat16_resolve(struct disk *disk);
void *fat16_open(struct disk *disk, struct path_part *path, file_mode mode);
size_t fat16_read(struct disk *disk, void *descriptor, uint32_t size, uint32_t nmemb, char *out_ptr);
int fat16_seek(void *private, uint32_t offset, file_seek_mode seek_mode);
int fat16_stat(struct disk *disk, void *private, struct file_stat *stat);

struct filesystem fat16_fs = {.resolve = fat16_resolve, .open = fat16_open, .read = fat16_read, .seek = fat16_seek, .stat = fat16_stat};

// Private prototypes
static int fat16_read_internal(const struct disk *disk, int starting_cluster, int offset, int total, void *out);

struct filesystem *fat16_init()
{
  strcpy(fat16_fs.name, "FAT16");
  return &fat16_fs;
}

static void fat16_init_private(struct disk *disk, struct fat_private *fprivate)
{
  memset(fprivate, 0, sizeof(struct fat_private));
  fprivate->cluster_read_stream = diskstream_new(disk->id);
  fprivate->fat_read_stream = diskstream_new(disk->id);
  fprivate->directory_stream = diskstream_new(disk->id);
}


void fat16_free_directory(struct fat_directory *directory)
{
  if (!directory) {
    return;
  }

  if (directory->item) {
    kfree(directory->item);
  }

  kfree(directory);
}

void fat16_fat_item_free(struct fat_item *item)
{
  if (!item) {
    return;
  }

  if (item->type == fat_type_directory) {
    fat16_free_directory(item->directory);
  } else if (item->type == fat_type_file) {
    kfree(item->item);
  } else {
    // uh?! TODO
  }

  kfree(item);
}

static int fat16_cluster_to_sector(const struct fat_private *private, int cluster)
{
  // Cluster index starts from 2 because the first two are used for metadata in the FAT
  int cluster_idx = cluster - 2;
  return private->root_directory.ending_sector_pos + (cluster_idx * private->header.primary_header.sectors_per_cluster);
}

int fat16_get_total_items_for_directory(const struct disk *disk, uint32_t directory_start_sector)
{
  struct fat_directory_item item;
  // struct fat_directory_item empty_item;
  // memset(&empty_item, 0, sizeof(empty_item));

  struct fat_private *fat_private = disk->fs_private;

  int res = 0;
  int directory_start_pos = directory_start_sector * disk->sector_size;

  // Move to the beginning of the directory data
  struct disk_stream *stream = fat_private->directory_stream;
  if (diskstream_seek(stream, directory_start_pos) != EOK) {
    return -EIO;
  }

  do {
    if (diskstream_read(stream, &item, sizeof(item)) != EOK) {
      return -EIO;
    }

    // skip unused items
    if (item.filename[0] == NUTSOS_FAT16_UNUSED_ENTRY) {
      continue;
    }

    res++;
  } while (item.filename[0] != NUTSOS_FAT16_NOMORE_ENTRY);

  // Remove last entry and return
  return --res;
}

// Load fat_directory from a fat_directory_item representing a directory
struct fat_directory *fat16_load_fat_directory(const struct disk *disk, const struct fat_directory_item *item)
{
  int res = 0;
  struct fat_directory *directory = 0;
  struct fat_private *fat_private = disk->fs_private;

  if (!(item->attribute & FAT_FILE_SUBDIRECTORY)) {
    // This is not a directory
    return ERRTOPTR(-EINVARG);
  }

  directory = kzalloc(sizeof(struct fat_directory));
  if (!directory) {
    res = -ENOMEM;
    goto cleanup;
  }

  int cluster = item->low_16_bits_first_cluster;
  int cluster_sector = fat16_cluster_to_sector(fat_private, cluster);

  res = fat16_get_total_items_for_directory(disk, cluster_sector);
  if (ISERR(res)) {
    goto cleanup;
  }
  directory->total = res;

  int directory_size = directory->total * sizeof(struct fat_directory_item);
  directory->item = kzalloc(directory_size);
  if (!directory->item) {
    res = -ENOMEM;
    goto cleanup;
  }

  res = fat16_read_internal(disk, cluster, 0x00, directory_size, directory->item);
  if (ISERR(res)) {
    goto cleanup;
  }

cleanup:
  if (ISERR(res)) {
    if (directory) {
      fat16_free_directory(directory);
    }
    return ERRTOPTR(res);
  }

  return directory;
}

// Deep copy of a directory_item
struct fat_directory_item *fat16_copy_directory_item(const struct fat_directory_item *item)
{
  struct fat_directory_item *copy = kzalloc(sizeof(struct fat_directory_item));
  if (!copy) {
    return NULL;
  }

  memcpy(copy, item, sizeof(struct fat_directory_item));
  return copy;
}

struct fat_item *fat16_new_fat_item_for_directory_item(const struct disk *disk, struct fat_directory_item *item)
{
  struct fat_item *fat_item = kzalloc(sizeof(struct fat_item));
  if (!fat_item) {
    return 0;
  }

  if (item->attribute & FAT_FILE_SUBDIRECTORY) {
    // It's dir - let's load its content from disk
    fat_item->directory = fat16_load_fat_directory(disk, item);
    fat_item->type = fat_type_directory;
  } else {
    // It's file - we're done
    fat_item->item = fat16_copy_directory_item(item);
    fat_item->type = fat_type_file;
  }

  return fat_item;
}


int fat16_sector_to_abs_address(const struct disk *disk, int sector)
{
  return sector * disk->sector_size;
}


void fat16_get_full_filename(const struct fat_directory_item *item, char *out, int max_len)
{
  max_len--; // Account for the mandatory nil-terminator

  // Copy the filename to out until we find a space (filenames are space padded)
  for (int i = 0; i < sizeof(item->filename) && max_len; i++, max_len--) {
    char c = item->filename[i];
    if (c == '\0' || isspace(c)) {
      break;
    }
    *out++ = c;
  }

  // No extension on directories
  if (item->attribute & FAT_FILE_SUBDIRECTORY) {
    *out = '\0';
    return;
  }

  // If there's no space for the extension let's not bother with the '.'
  if (--max_len == 0) {
    return;
  }

  // Add the extension separator
  *out++ = '.';

  // Ditto for extensions
  for (int i = 0; i < sizeof(item->ext) && max_len; i++, max_len--) {
    char c = item->ext[i];
    if (c == '\0' || isspace(c)) {
      break;
    }
    *out++ = c;
  }

  *out++ = '\0';
}

// Find a fat_item for file described by name
struct fat_item *fat16_find_item_in_directory(const struct disk *disk, const struct fat_directory *directory, const char *name)
{
  struct fat_item *f_item = 0;
  char tmp_filename[NUTSOS_MAX_PATH];
  for (int i = 0; i < directory->total; i++) {
    fat16_get_full_filename(&directory->item[i], tmp_filename, sizeof(tmp_filename));
    if (istrncmp(tmp_filename, name, sizeof(tmp_filename)) == 0) {
      // Found it let's create a new fat_item
      f_item = fat16_new_fat_item_for_directory_item(disk, &directory->item[i]);
      break;
    }
  }

  return f_item;
}

// return the fat_item for a directory described by path
struct fat_item *fat16_get_directory_entry(struct disk *disk, struct path_part *path)
{
  struct fat_private *fat_private = disk->fs_private;
  struct fat_item *current_item = NULL;
  struct fat_item *root_item = fat16_find_item_in_directory(disk, &fat_private->root_directory, path->part);
  if (!root_item) {
    // Can't find the root directory - path is invalid
    return NULL;
  }

  struct path_part *next_part = path->next;
  current_item = root_item;
  while (next_part) {
    if (current_item->type != fat_type_directory) {
      // we found an item that is not a directory - path is not describing a dir
      fat16_fat_item_free(current_item);
      current_item = NULL;
      break;
    }

    // Load items of the found subdir and continue
    struct fat_item *next_item = fat16_find_item_in_directory(disk, current_item->directory, next_part->part);
    if (!next_item) {
      // path is invalid as we can't find the next subdir
      fat16_fat_item_free(current_item);
      current_item = NULL;
      break;
    }
    fat16_fat_item_free(current_item);
    current_item = next_item;
    next_part = next_part->next;
  }

  return current_item;
}

// Give a fat16 cluster, retrieve the fat_entry
static int fat16_get_fat_entry_for_cluster(const struct disk *disk, int cluster)
{
  struct fat_private *private = disk->fs_private;
  struct disk_stream *stream = private->fat_read_stream;
  if (!stream) {
    return -EIO;
  }

  // Move the stream to the address of the cluster within the fat table
  int res = diskstream_seek(stream, private->fat_table_address + (cluster * NUTSOS_FAT16_FAT_ENTRY_SIZE));
  if (ISERR(res)) {
    return res;
  }

  uint16_t result = 0;
  res = diskstream_read(stream, &result, sizeof(result));
  if (ISERR(res)) {
    return res;
  }

  return result;
}

// find the cluster containing the file data at offset
static int fat16_get_cluster_for_offset(const struct disk *disk, int from_cluster, int offset)
{
  struct fat_private *private = disk->fs_private;
  int target_cluster = from_cluster;
  int offset_to_cluster = offset / private->bytes_per_cluster;

  // Walk the cluster chain to find the cluster containing the offset
  for (int i = 0; i < offset_to_cluster; i++) {
    int entry = fat16_get_fat_entry_for_cluster(disk, target_cluster);
    if (NUTSOS_FAT16_IS_LAST_SECTOR(entry)) {
      // We reached the end of the cluster chain before we should have!
      return -EIO;
    }

    if (entry == NUTSOS_FAT16_BAD_SECTOR) {
      // We reached a bad sector in the cluster-chain and can't proceed
      return -EIO;
    }

    if (NUTSOS_FAT16_IS_RESERVED_SECTOR(entry)) {
      // We reached a reserved sector in the cluster-chain and we can't proceed
      return -EIO;
    }

    if (entry == NUTSOS_FAT16_FREE_CLUSTER) {
      // We reached an empty cluster in our cluster-chain - something is not right!
      return -EIO;
    }

    target_cluster = entry;
  }

  return target_cluster;
}

static int fat16_read_internal(const struct disk *disk, int starting_cluster, int offset, int total, void *out)
{
  struct fat_private *private = disk->fs_private;
  struct disk_stream *stream = private->cluster_read_stream;
  int res = 0;
  int target_cluster = fat16_get_cluster_for_offset(disk, starting_cluster, offset);
  if (ISERR(target_cluster)) {
    return target_cluster;
  }

  // Where within the cluster do we need to start reading?
  int offset_from_cluster = offset % private->bytes_per_cluster;

  // Sector of the target_cluster
  int starting_sector = fat16_cluster_to_sector(private, target_cluster);

  // Absolute address to start to read from
  int starting_pos = (starting_sector * disk->sector_size) + offset_from_cluster;

  // Cap the reading to the remaining within the current cluster
  // We need to chunk our read as crossing cluster boundaries might mean moving to a different
  // section fo the disk to read the rest of the file
  int bytes_left_to_read_in_cluster = private->bytes_per_cluster - offset_from_cluster;
  int total_to_read = total > bytes_left_to_read_in_cluster ? bytes_left_to_read_in_cluster : total;

  res = diskstream_seek(stream, starting_pos);
  if (ISERR(res)) {
    return res;
  }

  res = diskstream_read(stream, out, total_to_read);
  if (ISERR(res)) {
    return res;
  }

  if (total > total_to_read) {
    // We've just read until the end of the current cluster - we ened to continue from the next cluster

    // Move the out buffer to the first free byte
    out += total_to_read;

    // Remove the bytes we've already read from total
    total -= total_to_read;

    // Recalculate the offset based on what we've already read
    offset += total_to_read;
    // We still have more to read

    // Shift the starting cluster to the one we've just read from
    starting_cluster = target_cluster;

    return fat16_read_internal(disk, starting_cluster, offset, total, out); // TODO: avoid recursion
  }

  return res;
}


int fat16_get_root_directory(struct disk *disk, struct fat_private *fat_private, struct fat_directory *directory_out)
{
  int res = 0;
  struct fat_header_main *primary_header = &fat_private->header.primary_header;
  int root_dir_sector_pos = (primary_header->fat_copies * primary_header->sectors_per_fat) + primary_header->reserved_sectors;
  int root_dir_entries = fat_private->header.primary_header.root_dir_entries;
  int root_dir_size = (root_dir_entries * sizeof(struct fat_directory_item));
  int total_root_sectors = root_dir_size / disk->sector_size;
  if (root_dir_size % disk->sector_size) {
    total_root_sectors += 1;
  }

  int total_items = fat16_get_total_items_for_directory(disk, root_dir_sector_pos);

  struct fat_directory_item *dir = kzalloc(root_dir_size);
  if (!dir) {
    return -ENOMEM;
  }

  struct disk_stream *stream = fat_private->directory_stream;
  if (diskstream_seek(stream, fat16_sector_to_abs_address(disk, root_dir_sector_pos)) != EOK) {
    return -EIO;
  }

  if (diskstream_read(stream, dir, root_dir_size) != EOK) {
    return -EIO;
  }

  directory_out->item = dir;
  directory_out->total = total_items;
  directory_out->sector_pos = root_dir_sector_pos;
  directory_out->ending_sector_pos = root_dir_sector_pos + total_root_sectors;

  return res;
}

int fat16_resolve(struct disk *disk)
{
  int res = 0;
  struct fat_private *fat_private = kzalloc(sizeof(struct fat_private));
  fat16_init_private(disk, fat_private);

  disk->fs_private = fat_private;
  disk->filesystem = &fat16_fs;

  struct disk_stream *stream = diskstream_new(disk->id);
  if (!stream) {
    res = -ENOMEM;
    goto out;
  }

  if (diskstream_read(stream, &fat_private->header, sizeof(fat_private->header)) != EOK) {
    res = -EIO;
    goto out;
  }

  if (fat_private->header.shared.extended_header.signature != NUTOS_FAT16_SIGNATURE) {
    res = -EINVALID;
    goto out;
  }

  // The first sector starts after the reserved ones
  fat_private->first_fat_sector = fat_private->header.primary_header.reserved_sectors;

  // The absolute address of the fat_table is at the beginning of the first fat sector (after reserved ones)
  fat_private->fat_table_address = fat_private->first_fat_sector * disk->sector_size;

  // pre calculate how many bytes in a cluster
  fat_private->bytes_per_cluster = fat_private->header.primary_header.sectors_per_cluster * disk->sector_size;
  if (fat16_get_root_directory(disk, fat_private, &fat_private->root_directory) != EOK) {
    res = -EIO;
    goto out;
  }

out:
  if (stream) {
    diskstream_close(stream);
  }

  if (ISERR(res)) {
    kfree(fat_private);
    disk->fs_private = 0;
  }
  return res;
}

void *fat16_open(struct disk *disk, struct path_part *path, file_mode mode)
{
  if (mode != FILE_MODE_READ) {
    return ERRTOPTR(-EREADONLY);
  }

  struct fat_file_descriptor *descriptor = kzalloc(sizeof(struct fat_file_descriptor));
  if (!descriptor) {
    return ERRTOPTR(-ENOMEM);
  }

  descriptor->item = fat16_get_directory_entry(disk, path);
  if (!descriptor->item) {
    kfree(descriptor);
    return ERRTOPTR(-EIO);
  }

  descriptor->pos = 0;
  return descriptor;
}

size_t fat16_read(struct disk *disk, void *descriptor, uint32_t size, uint32_t nmemb, char *out_ptr)
{
  int read = 0;
  struct fat_file_descriptor *fat_desc = descriptor;
  struct fat_directory_item *item = fat_desc->item->item;
  int offset = fat_desc->pos;
  // Do we have enough ?


  while (nmemb--) {
    // Do we have enough to read?
    if (size > item->filesize - offset) {
      return read;
    }

    if (ISERR(fat16_read_internal(disk, item->low_16_bits_first_cluster, offset, size, out_ptr))) {
      return read;
    }

    out_ptr += size;
    offset += size;
    read++;
  }

  return read;
}

int fat16_seek(void *private, uint32_t offset, file_seek_mode seek_mode)
{
  struct fat_file_descriptor *desc = private;
  struct fat_item *desc_item = desc->item;
  if (desc_item->type != fat_type_file) {
    return -EINVARG;
  }

  struct fat_directory_item *ritem = desc_item->item;

  switch (seek_mode) {
  case SEEK_SET:
    if (offset >= ritem->filesize) {
      return -EIO;
    }
    desc->pos = offset;
    break;

  case SEEK_END:
    return -1;
    break;

  case SEEK_CUR:
    if (desc->pos + offset >= ritem->filesize) {
      return -EIO;
    }
    desc->pos += offset;
    break;

  default:
    return -EINVARG;
    break;
  }

  return EOK;
}

int fat16_stat(struct disk *disk, void *private, struct file_stat *stat)
{
  struct fat_file_descriptor *descriptor = (struct fat_file_descriptor *)private;
  struct fat_item *desc_item = descriptor->item;
  if (desc_item->type != fat_type_file) {
    return -EINVARG;
  }

  struct fat_directory_item *ritem = desc_item->item;
  stat->filesize = ritem->filesize;
  stat->flags = 0;

  if (ritem->attribute & FAT_FILE_READ_ONLY) {
    stat->flags |= FILE_STAT_RO;
  }
  return 0;
}