#ifndef FILE_H
#define FILE_H

#include "path_parser.h"
#include <stddef.h>
#include <stdint.h>

typedef enum
{
  SEEK_SET,
  SEEK_CUR,
  SEEK_END
} file_seek_mode;

typedef enum
{
  FILE_MODE_READ,
  FILE_MODE_WRITE,
  FILE_MODE_APPEND,
  FILE_MODE_INVALID
} file_mode;

struct disk;

typedef void *(*fs_open_function_t)(struct disk *disk, struct path_part *path, file_mode mode);
typedef int (*fs_resolve_function_t)(struct disk *disk);
typedef size_t (*fs_read_function_t)(struct disk *disk, void *private, uint32_t size, uint32_t nmemb, char *out);
typedef int (*fs_seek_function_t)(void *private, uint32_t offset, file_seek_mode seek_mode);


struct filesystem {
  // Filesystem should return zero from resolve if the provided disk is using its filesystem
  fs_resolve_function_t resolve;
  fs_open_function_t open;
  fs_read_function_t read;
  fs_seek_function_t fseek;

  char name[20];
};

struct file_descriptor {
  // The descriptor index
  int index;
  struct filesystem *filesystem;

  // Private data for internal file descriptor
  void *private;

  file_mode mode;

  // The disk that the file descriptor should be used on
  struct disk *disk;
};

void fs_init();
file_mode file_mode_from_string(const char *str);
struct file_descriptor *fopen(const char *filename, const char *mode_str);
size_t fread(void *ptr, uint32_t size, uint32_t nmemb, int fd);
int fseek(int fd, int offset, file_seek_mode mode);
int fs_insert_filesystem(struct filesystem *filesystem);
struct filesystem *fs_resolve(struct disk *disk);

#endif