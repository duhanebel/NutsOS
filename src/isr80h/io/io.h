#ifndef ISR80H_IO_H
#define ISR80H_IO_H

struct interrupt_frame;
void *isr80h_command_print(struct interrupt_frame *frame);
#endif