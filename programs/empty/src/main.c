const char local_msg[] = "Empty program is running yo!";
void main() {
  asm volatile("mov $1, %%eax\n\t"
               "push %[msg]\n\t"
               "int $0x80\n\t"
               "add $8, %%esp"
               : /*no_out*/
               : [msg] "r"(local_msg)
               : "eax");
  while (1) {
  }
}