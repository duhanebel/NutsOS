#include "path_parser.h"
#include "config.h"
#include "error.h"
#include "kernel.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "stdutil/string.h"

static int pathparser_is_path_valid(const char *filename)
{
  int len = strnlen(filename, NUTSOS_MAX_PATH);
  return (len >= 3 && isdigit(filename[0]) && memcmp((void *)&filename[1], ":/", 2) == 0);
}

static int pathparser_extract_drive_from_path(const char **path)
{
  if (!pathparser_is_path_valid(*path)) {
    return -EBADPATH;
  }

  int drive_no = ctoi(*path[0]);

  // Add 3 bytes to skip drive number 0:/ 1:/ 2:/
  *path += 3;
  return drive_no;
}

static struct path_root *pathparser_create_root(int drive_number)
{
  struct path_root *path_r = kzalloc(sizeof(struct path_root));
  path_r->drive_no = drive_number;
  path_r->first = 0;
  return path_r;
}

static const char *pathparser_extract_next_path_part(const char **path)
{
  char *result_path_part = kzalloc(NUTSOS_MAX_PATH);
  int i = 0;
  while (**path != '/' && **path != 0) {
    result_path_part[i] = **path;
    *path += 1;
    i++;
  }

  if (**path == '/') {
    *path += 1;
  }

  if (i == 0) {
    kfree(result_path_part);
    result_path_part = 0;
  }

  return result_path_part;
}

struct path_part *pathparser_parse_path_part(struct path_part *last_part, const char **path)
{
  const char *path_part_str = pathparser_extract_next_path_part(path);
  if (!path_part_str) {
    return 0;
  }

  struct path_part *part = kzalloc(sizeof(struct path_part));
  part->part = path_part_str;
  part->next = 0;

  if (last_part) {
    last_part->next = part;
  }

  return part;
}

void pathparser_free(struct path_root *root)
{
  struct path_part *part = root->first;
  while (part) {
    struct path_part *next_part = part->next;
    kfree((void *)part->part);
    kfree(part);
    part = next_part;
  }

  kfree(root);
}

struct path_root *pathparser_parse(const char *path, const char *current_directory_path)
{
  int res = 0;
  const char *tmp_path = path;
  struct path_root *path_root = 0;

  if (strlen(path) > NUTSOS_MAX_PATH) {
    return path_root;
  }

  res = pathparser_extract_drive_from_path(&tmp_path);
  if (res < 0) {
    return path_root;
  }

  path_root = pathparser_create_root(res);
  if (!path_root) {
    return path_root;
  }

  struct path_part *first_part = pathparser_parse_path_part(NULL, &tmp_path);
  if (!first_part) {
    return path_root;
  }

  path_root->first = first_part;
  struct path_part *part = pathparser_parse_path_part(first_part, &tmp_path);
  while (part) {
    part = pathparser_parse_path_part(part, &tmp_path);
  }

  return path_root;
}