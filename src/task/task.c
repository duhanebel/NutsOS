#include "task.h"
#include "config.h"
#include "error.h"
#include "kernel.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"

// The current task that is running
struct task *current_task = 0;

// Task linked list
struct task *task_tail = 0;
struct task *task_head = 0;

int task_init(struct task *task);

struct task *task_new()
{
  int res = 0;
  struct task *task = kzalloc(sizeof(struct task));
  if (!task) {
    res = -ENOMEM;
    goto out;
  }

  res = task_init(task);
  if (ISERR(res)) {
    goto out;
  }

  if (task_head == 0) {
    // Is this the first task?
    task_head = task;
    task_tail = task;
    goto out;
  } else {
    // Otherwise add it to the end
    task_tail->next = task;
    task->prev = task_tail;
    task_tail = task;
  }

out:
  if (ISERR(res)) {
    task_free(task);
    return ERRTOPTR(res);
  }

  return task;
}

struct task *task_get_next()
{
  if (!current_task->next) {
    return task_head;
  }

  return current_task->next;
}

static void task_list_remove(struct task *task)
{
  if (task->prev) {
    task->prev->next = task->next;
  }

  if (task == task_head) {
    task_head = task->next;
  }

  if (task == task_tail) {
    task_tail = task->prev;
  }

  if (task == current_task) {
    current_task = task_get_next();
  }
}

int task_free(struct task *task)
{
  paging_chunk_free(task->page_directory);
  task_list_remove(task);

  // Finally free the task data
  kfree(task);
  return 0;
}

int task_init(struct task *task)
{
  memset(task, 0, sizeof(struct task));
  // Map the entire 4GB address space to its self
  task->page_directory = paging_chunk_new(PAGING_TOTAL_DIR_ENTRIES, PAGING_TOTAL_ENTRIES_PER_TABLE, PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
  if (!task->page_directory) {
    return -EIO;
  }

  task->registers.ip = NUTSOS_PROGRAM_VIRTUAL_ADDRESS;
  task->registers.ss = NUTSOS_GDT_USER_DATA_OFFSET;
  task->registers.esp = NUTSOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;

  return 0;
}