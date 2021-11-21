#ifndef PROCESS_H
#define PROCESS_H
#include "config.h"
#include "task.h"
#include <stdint.h>

struct process {
  // The process id
  uint16_t id;

  char filename[NUTSOS_MAX_PATH];

  // The main process task
  struct task *task;

  // The memory (malloc) allocations of the process
  void *allocations[1024]; // TODO convert this to a linked list of zones

  // The physical pointer to the process memory.
  void *ptr;

  // The physical pointer to the stack memory
  void *stack;

  // The size of the data pointed to by "ptr"
  uint32_t size;
};
#endif