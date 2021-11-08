#ifndef FILE_H
#define FILE_H

#include "path_parser.h"

typedef enum { SEEK_SET, SEEK_CUR, SEEK_END } file_seek_mode;

typedef enum { FILE_MODE_READ, FILE_MODE_WRITE, FILE_MODE_APPEND, FILE_MODE_INVALID } file_mode;

struct disk;
typedef void *(*fs_open_function_t)(struct disk *disk, struct path_part *path, file_mode mode);
typedef int (*fs_resolve_function_t)(struct disk *disk);

struct filesystem {
  // Filesystem should return zero from resolve if the provided disk is using its filesystem
  fs_resolve_function_t resolve;
  fs_open_function_t open;

  char name[20];
};

struct file_descriptor {
  // The descriptor index
  int index;
  struct filesystem *filesystem;

  // Private data for internal file descriptor
  void *private;

  // The disk that the file descriptor should be used on
  struct disk *disk;
};

void fs_init();
int fopen(const char *filename, const char *mode);
int fs_insert_filesystem(struct filesystem *filesystem);
struct filesystem *fs_resolve(struct disk *disk);

#endif