#ifndef TASK_H
#define TASK_H

#include "config.h"
#include "memory/paging/paging.h"
#include "task/process.h"

struct registers {
  // restore_general_purpose_registers managed
  uint32_t edi; // 0
  uint32_t esi; // 4
  uint32_t ebp; // 8
  uint32_t ebx; // 12
  uint32_t edx; // 16
  uint32_t ecx; // 20
  uint32_t eax; // 24

  // task_return managed
  uint32_t ip;    // 28
  uint32_t cs;    // 32
  uint32_t flags; // 36
  uint32_t esp;   // 40
  uint32_t ss;    // 44
};

struct task {
  /**
      * The page directory of the task
      */
  struct paging_chunk *page_directory;

  // The registers of the task when the task is not running
  struct registers registers;

  // The process of the task
  struct process *process;

  // The next task in the linked list
  struct task *next;

  // Previous task in the linked list
  struct task *prev;
};

struct task *task_new(struct process *process);

struct task *task_get_next();
void task_free(struct task *task);
void task_run_as_task0(struct task *task);

struct interrupt_frame;
void task_current_save_state(struct interrupt_frame *frame);
void *task_current_get_stack_item(int index);
bool task_current_validate_pointer(void *ptr);
#endif