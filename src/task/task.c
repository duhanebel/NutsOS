#include "task.h"
#include "config.h"
#include "error.h"
#include "idt/idt.h"
#include "kernel.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"

extern void set_gdt_segments(uint8_t index);
extern void task_return();

// The current task that is running
struct task *current_task = 0;

// Task linked list
struct task *task_tail = 0;
struct task *task_head = 0;

static int task_init(struct task *task, struct process *process);

struct task *task_new(struct process *process)
{
  int res = 0;
  struct task *task = kzalloc(sizeof(struct task));
  if (!task) {
    res = -ENOMEM;
    goto out;
  }

  res = task_init(task, process);
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
    task->next->prev = task->prev;
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

void task_free(struct task *task)
{
  paging_chunk_free(task->page_directory);
  task_list_remove(task);

  // Finally free the task data
  kfree(task);
}

static int task_init(struct task *task, struct process *process)
{
  memset(task, 0, sizeof(struct task));
  // Give 4GB of paging to this task
  task->page_directory = paging_chunk_new(PAGING_TOTAL_DIR_ENTRIES, PAGING_TOTAL_ENTRIES_PER_TABLE, PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
  if (!task->page_directory) {
    return -EIO;
  }

  task->registers.ip = NUTSOS_PROGRAM_VIRTUAL_ADDRESS;
  task->registers.ss = GDT_USER_DATA_OFFSET | GDT_PRIVILEGE_RING_3;
  task->registers.cs = GDT_USER_CODE_OFFSET | GDT_PRIVILEGE_RING_3;
  task->registers.esp = NUTSOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;
  task->process = process;

  return 0;
}

int task_switch(struct task *task)
{
  current_task = task;
  paging_switch(task->page_directory->directory_entry);
  return 0;
}

// int task_page()
// {
//   // We need to set the gdt segments
//   set_gdt_segments(NUTSOS_GDT_USER_DATA_OFFSET | NUTSOS_GDT_PRIVILEGE_RING_3);
//   task_switch(current_task);
//   return 0;
// }

void task_run_as_task0(struct task *task)
{
  if (task != task_head) {
    panic("Can't run a first task that is not the first task!!\n");
  }

  task_switch(task);
  task_return(&task->registers);
}

// Store the state of the interrupt_frame into the registers of the task' struct
void task_save_state(struct task *task, struct interrupt_frame *frame)
{
  task->registers.ip = frame->ip;
  task->registers.cs = frame->cs;
  task->registers.flags = frame->flags;
  task->registers.esp = frame->esp;
  task->registers.ss = frame->ss;
  task->registers.eax = frame->eax;
  task->registers.ebp = frame->ebp;
  task->registers.ebx = frame->ebx;
  task->registers.ecx = frame->ecx;
  task->registers.edi = frame->edi;
  task->registers.edx = frame->edx;
  task->registers.esi = frame->esi;
}

// Store the state of the interrupt frame for the current running task
void task_current_save_state(struct interrupt_frame *frame)
{
  if (current_task == NULL) {
    panic("No current task to save\n");
  }

  task_save_state(current_task, frame);
}

// Get the stack item at index from the current task
void *task_current_get_stack_item(int index)
{
  if (current_task == NULL) {
    panic("No current task to save\n");
  }

  struct task *task = current_task;
  uint32_t *sp_ptr = (uint32_t *)task->registers.esp;

  return (void *)sp_ptr[index];
}

// Check if pointer is valid for memory mapped by current task
bool task_current_validate_pointer(void *ptr)
{
  struct task *task = current_task;
  return process_validate_pointer(task->process, ptr);
}