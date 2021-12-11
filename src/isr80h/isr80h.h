#ifndef ISR80H_H
#define ISR80H_H

typedef enum
{
  INT80H_COMMAND_SUM,
  INT80H_COMMAND_PRINT,
  INT80H_COMMMAND_MAX
} int80h_commands_t;

void isr80h_register_commands();

struct interrupt_frame;
void *isr80h_handle_command(int command, struct interrupt_frame *frame);


#endif