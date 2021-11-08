#include "stream.h"
#include "config.h"
#include "error.h"
#include "memory/heap/kheap.h"

struct disk_stream *diskstream_new(int disk_id) {
  struct disk *disk = disk_get(disk_id);
  if (!disk) {
    return 0;
  }

  struct disk_stream *stream = kzalloc(sizeof(struct disk_stream));
  stream->pos = 0;
  stream->disk = disk;
  return stream;
}

int diskstream_seek(struct disk_stream *stream, int pos) {
  stream->pos = pos;
  return 0;
}

int diskstream_read_old(struct disk_stream *stream, void *out, int total) {
  int sector = stream->pos / NUTSOS_SECTOR_SIZE;
  int offset = stream->pos % NUTSOS_SECTOR_SIZE;
  char buf[NUTSOS_SECTOR_SIZE];

  int res = disk_read_block(stream->disk, sector, 1, buf);
  if (res < 0) {
    goto out;
  }

  int total_to_read = total > NUTSOS_SECTOR_SIZE ? NUTSOS_SECTOR_SIZE : total;
  for (int i = 0; i < total_to_read; i++) {
    *(char *)out++ = buf[offset + i];
  }

  // Adjust the stream
  stream->pos += total_to_read;
  if (total > NUTSOS_SECTOR_SIZE) {
    res = diskstream_read(stream, out, total - NUTSOS_SECTOR_SIZE);
  }
out:
  return res;
}

int diskstream_read(struct disk_stream *stream, void *out, int total) {
  char buf[NUTSOS_SECTOR_SIZE];

  int sector = stream->pos / NUTSOS_SECTOR_SIZE;
  int offset = stream->pos % NUTSOS_SECTOR_SIZE;
  int readable_in_sector = NUTSOS_SECTOR_SIZE - offset;
  int remaining_to_read = total;
  int total_to_read = remaining_to_read > readable_in_sector ? readable_in_sector : remaining_to_read;
  int res = EOK;
  do {
    res = disk_read_block(stream->disk, sector, 1, buf);
    if (res < 0) {
      break;
    }

    int read = total_to_read;
    while (read-- > 0) {
      *(char *)out++ = buf[offset++];
    }

    // Adjust the stream
    stream->pos += total_to_read;
    sector = stream->pos / NUTSOS_SECTOR_SIZE;
    offset = stream->pos % NUTSOS_SECTOR_SIZE;
    readable_in_sector = NUTSOS_SECTOR_SIZE - offset;
    remaining_to_read -= total_to_read;
    total_to_read = remaining_to_read > readable_in_sector ? readable_in_sector : remaining_to_read;

  } while (remaining_to_read > 0);

  return res;
}

void diskstream_close(struct disk_stream *stream) { kfree(stream); }