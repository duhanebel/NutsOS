#include "misc.h"
#include "idt/idt.h"

void *isr80h_command_sum(struct interrupt_frame *frame)
{
  return (void *)10;
}