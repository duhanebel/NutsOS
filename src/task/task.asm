[BITS 32]
 section .asm

 global restore_general_purpose_registers
 global task_return
 global set_gdt_segments

 ; void task_return(struct registers* regs)
 ; Sets up the stack to return to ring3 with the registers passed as a param
 ; ebp+4: registers struct
 task_return:
     mov ebp, esp ; no need to preserve ebp as we're never returning to the caller
     mov ebx, [ebp+4] ; ebx = registers struct

    ; We need to prepare the stack as iret expexts it. 
     push dword [ebx+44] ; push stack selector (ss)
     push dword [ebx+40] ; push stack pointer (esp)

     ; Push the flags
     pushf
     pop eax
     or eax, 0x200 ; turn on interrupts after iret -- NO: Change the exection flag to ring3
     push eax

     push dword [ebx+32] ; push code selector (cs)
     push dword [ebx+28] ; push the instruction pointer (ip)

    ; reset the other segment selectors to be the same as ss
     mov ax, [ebx+44]
     mov ds, ax
     mov es, ax
     mov fs, ax
     mov gs, ax

    ; restore general registers
     push dword ebx ; push on the stack the pointer to registers struct
     call restore_general_purpose_registers ; restore the general purpose registers
     add esp, 4 ; clean the stack for the previous call

     iretd; Return to ring3
     
 ; void restore_general_purpose_registers(struct registers* regs)
 ; Restores CPU general registers from a registers struct
 ; esp+4: pointer to registers struct
 restore_general_purpose_registers:
     mov ebx, [esp+4] ; ebx = struct registers
     
     mov edi, [ebx]
     mov esi, [ebx+4]
     mov ebp, [ebx+8]
     ; ebx goes last
     mov edx, [ebx+16]
     mov ecx, [ebx+20]
     mov eax, [ebx+24]

     mov ebx, [ebx+12] ; this goes last because we've been using it to index into the registers struct
     ret

 ; void set_gdt_segments(uint8_t gdt_index)
 ; Loads the same gtd segment for ds, es, fs and gs
 ; ebp+8: index in the gdt table with the segment to load
 set_gdt_segments:
     push ebp
     mov ebp, esp
     mov eax, [ebp+8]
     mov ds, ax
     mov es, ax
     mov fs, ax
     mov gs, ax
     pop ebp
     ret 