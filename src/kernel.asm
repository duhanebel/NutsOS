[BITS 32]

global _start
extern kmain

CODE_SEG equ 0x08 ; see boot.asm - code segment was set up at offset 8
DATA_SEG equ 0x10 ; and data at offset 10 of the gdt table

_start:
    mov ax, DATA_SEG
    mov ds, ax ; set up all other segment to overlap with data
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp ; initialize the stack pointer to a decent value (2MB off)

    ; Enable the A20 line - necessary to address >= 1MB RAM
    ; see https://en.wikipedia.org/wiki/A20_line for more info
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Remap the master PIC
    ; In protected mode, the IRQs 0 to 7 conflict with the CPU exception which are reserved 
    ; by Intel up until 0x1F. (It was an IBM design mistake.) Consequently it is difficult 
    ; to tell the difference between an IRQ or an software error. It is thus recommended to 
    ; change the PIC's offsets (also known as remapping the PIC) so that IRQs use non-reserved vectors. 
    ; A common choice is to move them to the beginning of the available range (IRQs 0..0xF -> INT 0x20..0x2F). 
    mov al, 00010001b
    out 0x20, al ; Tell master PIC - select master PIC

    mov al, 0x20 ; Interrupt 0x20 is where master ISR should start
    out 0x21, al ; send the new base 0x20 to the PIC

    mov al, 00000001b
    out 0x21, al ; set PIC mode to 8086/88 (MCS-80/85)
    ; End remap of the master PIC

    ; jump onto C code
    call kmain

    jmp $ ; should never reach here but loop on the spot if kmain ever returns
