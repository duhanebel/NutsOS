#include "io.h"
#include "task/task.h"
#include "terminal/terminal.h"

void *isr80h_command_print(struct interrupt_frame *frame)
{
  void *msg_ptr = (char *)task_current_get_stack_item(0);
  if (!task_current_validate_pointer(msg_ptr)) {
    return (void *)-1;
  }

  print(msg_ptr);

  return 0;
}