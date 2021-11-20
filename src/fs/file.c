#include "file.h"
#include "config.h"
#include "disk/disk.h"
#include "error.h"
#include "fat/fat16.h"
#include "kernel.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "stdutil/string.h"

struct filesystem *filesystems[NUTSOS_MAX_FILESYSTEMS];
struct file_descriptor *file_descriptors[NUTSOS_MAX_FILE_DESCRIPTORS];

static struct filesystem **fs_get_free_filesystem()
{
  int i = 0;
  for (i = 0; i < NUTSOS_MAX_FILESYSTEMS; i++) {
    if (filesystems[i] == 0) {
      return &filesystems[i];
    }
  }

  return 0;
}

int fs_insert_filesystem(struct filesystem *filesystem)
{
  struct filesystem **fs;
  fs = fs_get_free_filesystem();
  if (!fs) {
    return -ENOMEM;
  }

  *fs = filesystem;
  return 0;
}

static void fs_static_load()
{
  fs_insert_filesystem(fat16_init());
}

void fs_load()
{
  fs_static_load();
}

void fs_init()
{
  memset(file_descriptors, 0, sizeof(file_descriptors));
  memset(filesystems, 0, sizeof(filesystems));

  fs_load();
}

static int file_descriptor_new(struct file_descriptor **desc_out)
{
  int res = -ENOMEM;
  for (int i = 0; i < NUTSOS_MAX_FILE_DESCRIPTORS; i++) {
    if (file_descriptors[i] == 0) {
      struct file_descriptor *desc = kzalloc(sizeof(struct file_descriptor));
      // Descriptors start at 1
      desc->index = i + 1;
      file_descriptors[i] = desc;
      *desc_out = desc;
      res = EOK;
      break;
    }
  }

  return res;
}

static struct file_descriptor *file_get_descriptor(int fd)
{
  if (fd <= 0 || fd >= NUTSOS_MAX_FILE_DESCRIPTORS) {
    return 0;
  }

  // Descriptors start at 1
  int index = fd - 1;
  return file_descriptors[index];
}

struct filesystem *fs_resolve(struct disk *disk)
{
  struct filesystem *fs = 0;
  for (int i = 0; i < NUTSOS_MAX_FILESYSTEMS; i++) {
    if (filesystems[i] != 0 && filesystems[i]->resolve(disk) == 0) {
      fs = filesystems[i];
      break;
    }
  }
  return fs;
}

file_mode file_mode_from_string(const char *str)
{
  file_mode mode = FILE_MODE_INVALID;
  if (strncmp(str, "r", 1) == 0) {
    mode = FILE_MODE_READ;
  } else if (strncmp(str, "w", 1) == 0) {
    mode = FILE_MODE_WRITE;
  } else if (strncmp(str, "a", 1) == 0) {
    mode = FILE_MODE_APPEND;
  }
  return mode;
}

struct file_descriptor *fopen(const char *filename, const char *mode_str)
{
  struct path_root *root_path = pathparser_parse(filename, NULL);
  if (!root_path) {
    errno = -EINVARG;
    return NULL;
  }

  // We cannot have just a root path 0:/ 0:/test.txt
  if (!root_path->first) {
    errno = -EINVARG;
    return NULL;
  }

  // Ensure the disk we are reading from exists
  struct disk *disk = disk_get(root_path->drive_no);
  if (!disk) {
    errno = -EIO;
    return NULL;
  }

  if (!disk->filesystem) {
    errno = -EIO;
    return NULL;
  }

  file_mode mode = file_mode_from_string(mode_str);
  if (mode == FILE_MODE_INVALID) {
    errno = -EINVARG;
    return NULL;
  }

  void *descriptor_private_data = disk->filesystem->open(disk, root_path->first, mode);
  if (ISERR(descriptor_private_data)) {
    errno = (int)(descriptor_private_data);
    return NULL;
  }

  struct file_descriptor *fdesc = NULL;
  errno = file_descriptor_new(&fdesc);
  if (ISERR(errno)) {
    return NULL;
  }

  fdesc->filesystem = disk->filesystem;
  fdesc->private = descriptor_private_data;
  fdesc->mode = mode;
  fdesc->disk = disk;

  return fdesc;
}

size_t fread(void *ptr, uint32_t size, uint32_t nmemb, int fd)
{
  if (size == 0 || nmemb == 0 || fd < 1) {
    return -EINVARG;
  }

  struct file_descriptor *desc = file_get_descriptor(fd);
  if (!desc) {
    return -EINVARG;
  }

  return desc->filesystem->read(desc->disk, desc->private, size, nmemb, (char *)ptr);
}

int fseek(int fd, int offset, file_seek_mode mode)
{
  struct file_descriptor *desc = file_get_descriptor(fd);
  if (!desc) {
    return -EIO;
  }

  return desc->filesystem->seek(desc->private, offset, mode);
}

int fstat(int fd, struct file_stat *stat)
{
  struct file_descriptor *desc = file_get_descriptor(fd);
  if (!desc) {
    return -EIO;
  }

  return desc->filesystem->stat(desc->disk, desc->private, stat);
}