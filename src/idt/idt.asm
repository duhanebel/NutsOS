section .asm

extern int21h_handler
extern no_interrupt_handler
extern isr80h_handler

global int21h
global idt_load
global no_interrupt
global enable_interrupts
global disable_interrupts
global isr80h_wrapper

; Enable interrupts
enable_interrupts:
    sti
    ret

; Disable interrupts
disable_interrupts:
    cli
    ret

; load the interrupt descriptor table
; ebp+8 = pointer to the IDT
idt_load:
    push ebp
    mov ebp, esp

    mov ebx, [ebp+8]
    lidt [ebx]
    pop ebp
    ret

int21h:
    pushad
    call int21h_handler
    popad
    iret

no_interrupt:
    pushad
    call no_interrupt_handler
    popad
    iret

; This function pushes the general registers into the interrupt frame and then call the handler
; It also deals with clearing up the stack after collecting the result from the handler function
isr80h_wrapper:
     ; INTERRUPT FRAME START
     ; The following have already been pushed by the cpu upon INT: ip, cs, flags, sp, ss
     pushad ; Pushes the general purpose registers to the stack
     
     push esp ; Push the stack pointer so that we are pointing to the interrupt frame

     push eax      ; EAX holds our command lets push it to the stack for isr80h_handler
     call isr80h_handler
     mov dword[tmp_res], eax ; store the result of irs80h_handler into a temp var
     add esp, 8 ; reset the stack to before the function call

     popad ; restore the registers to the one of the caller (the usermode process) - eax at this point is overwritten
     mov eax, [tmp_res] ; restore eax with the result from irs80h_handler
     iretd ; return to the process

 section .data
 tmp_res: dd 0 ; temporary data to store the result of isr80h_handler temporarily