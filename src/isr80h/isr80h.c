#include "isr80h.h"
#include "idt/idt.h"
#include "io/io.h"
#include "kernel.h"
#include "misc/misc.h"
#include "task/task.h"

// Type of a INT80 command
typedef void *(*isr80h_command)(struct interrupt_frame *frame);

// Stores all the functions implmementing the INT80 commands
static isr80h_command isr80h_commands[INT80H_COMMMAND_MAX];

// Handler for the INT80 (used for kernel call from user space)
// command: command number
// interrupt_frame: frame of the caller
void *isr80h_handle_command(int command, struct interrupt_frame *frame)
{
  void *result = 0;

  if (command < 0 || command >= INT80H_COMMMAND_MAX) {
    // Invalid command
    return 0;
  }

  isr80h_command command_func = isr80h_commands[command];
  if (!command_func) {
    // The function is not implemented, return
    return 0;
  }

  result = command_func(frame);
  return result;
}


// Register a new int80 command
// command_id a int80h_commands ID for the command to perform
// command the function responsible to execute
// panic if the command is already registered
void isr80h_register_command(int80h_commands_t command_id, isr80h_command command)
{
  if (command_id < 0 || command_id >= INT80H_COMMMAND_MAX) {
    panic("The command is out of bounds\n");
  }

  if (isr80h_commands[command_id]) {
    panic("Your attempting to overwrite an existing command\n");
  }

  isr80h_commands[command_id] = command;
}

// Registers all the supported isr80h commands
void isr80h_register_commands()
{
  isr80h_register_command(INT80H_COMMAND_SUM, isr80h_command_sum);
  isr80h_register_command(INT80H_COMMAND_PRINT, isr80h_command_print);
}
