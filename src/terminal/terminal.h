#ifndef TERMINAL_H
#define TERMINAL_H

#define VGA_WIDTH  80
#define VGA_HEIGHT 20

void terminal_initialize();
void print(const char *str);
void printf(const char *fmt, ...);

#endif