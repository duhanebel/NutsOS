#include "process.h"
#include "config.h"
#include "error.h"
#include "fs/file.h"
#include "kernel.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"
#include "stdutil/string.h"
#include "task/task.h"


// The current process that is running
struct process *current_process = 0;

static struct process *processes[NUTSOS_MAX_PROCESSES] = {};

static int process_load_for_slot(const char *filename, struct process **out, int process_slot);

static void process_init(struct process *process)
{
  memset(process, 0, sizeof(struct process));
}

struct process *process_get(int process_id)
{
  if (process_id < 0 || process_id >= NUTSOS_MAX_PROCESSES) {
    return ERRTOPTR(-EINVARG);
  }

  return processes[process_id];
}

static int process_load_binary(const char *filename, struct process *process)
{
  int res = 0;
  struct file_descriptor *fd = fopen(filename, "r");
  if (!fd) {
    res = -EIO;
    goto out;
  }

  struct file_stat stat;
  res = fstat(fd->index, &stat);
  if (ISERR(res)) {
    goto out;
  }

  void *program_data_ptr = kzalloc(stat.filesize);
  if (!program_data_ptr) {
    res = -ENOMEM;
    goto out;
  }

  if (fread(program_data_ptr, stat.filesize, 1, fd->index) != 1) {
    res = -EIO;
    goto out;
  }

  process->ptr = program_data_ptr;
  process->size = stat.filesize;

out:
  fclose(fd->index);
  return res;
}

static int process_load_data(const char *filename, struct process *process)
{
  int res = 0;
  res = process_load_binary(filename, process);
  return res;
}

int process_map_binary(struct process *process)
{
  return paging_map_to(process->task->page_directory->directory_entry,
                       (void *)NUTSOS_PROGRAM_VIRTUAL_ADDRESS,
                       process->ptr,
                       paging_align_address(process->ptr + process->size),
                       PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITEABLE);
}

int process_map_memory(struct process *process)
{
  int res = 0;
  res = process_map_binary(process);
  return res;
}

int process_load(const char *filename, struct process **process)
{
  int process_slot = -1;
  for (int i = 0; i < NUTSOS_MAX_PROCESSES; i++) {
    if (processes[i] == 0) {
      process_slot = i;
      break;
    }
  }

  if (process_slot < 0) {
    return -ENOMEM;
  }

  return process_load_for_slot(filename, process, process_slot);
}

static int process_load_for_slot(const char *filename, struct process **out, int process_slot)
{
  int res = 0;
  struct task *task = NULL;
  struct process *process = NULL;
  void *program_stack_ptr = NULL;

  if (process_get(process_slot) != 0) {
    res = -ETAKEN;
    goto out;
  }

  process = kzalloc(sizeof(struct process));
  if (!process) {
    res = -ENOMEM;
    goto out;
  }

  process_init(process);
  res = process_load_data(filename, process);
  if (res < 0) {
    goto out;
  }

  program_stack_ptr = kzalloc(NUTSOS_USER_PROGRAM_STACK_SIZE);
  if (!program_stack_ptr) {
    res = -ENOMEM;
    goto out;
  }

  strncpy(process->filename, filename, sizeof(process->filename));
  process->stack = program_stack_ptr;
  process->id = process_slot;

  // Create a task
  task = task_new(process);
  if (ISERR(task)) {
    res = PTRTOERR(task);
  }

  process->task = task;

  res = process_map_memory(process);
  if (ISERR(res)) {
    goto out;
  }

  *out = process;

  // Add the process to the array
  processes[process_slot] = process;

out:
  if (ISERR(res)) {
    if (process && process->task) {
      task_free(process->task);
    }

    // Free the process data
  }
  return res;
}