section .asm

global tss_load

; Load the tss segment passed as parameter
tss_load:
    push ebp
    mov ebp, esp
    mov ax, [ebp+8] ; TSS Segment
    ltr ax
    pop ebp
    ret