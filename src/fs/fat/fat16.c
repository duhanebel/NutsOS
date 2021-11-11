#include "fat16.h"
#include "error.h"
#include "stdutil/string.h"

int fat16_resolve(struct disk *disk);
void *fat16_open(struct disk *disk, struct path_part *path, file_mode mode);

struct filesystem fat16_fs = {.resolve = fat16_resolve, .open = fat16_open};

struct filesystem *fat16_init()
{
  strcpy(fat16_fs.name, "FAT16");
  return &fat16_fs;
}

// Only support one disk atm
int fat16_resolve(struct disk *disk)
{
  return 0;
}

void *fat16_open(struct disk *disk, struct path_part *path, file_mode mode)
{
  return 0;
}