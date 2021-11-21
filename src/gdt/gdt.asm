section .asm
 global gdt_load

; load a gdt table on the cpu
; arg1 - gdt vector
; arg2 - gdt size
 gdt_load:
     mov eax, [esp+4]
     mov [gdt_descriptor + 2], eax
     mov ax, [esp+8]
     mov [gdt_descriptor], ax
     lgdt [gdt_descriptor]
     ret

 section .data
 gdt_descriptor:
     dw 0x00 ; Size
     dd 0x00 ; GDT Start Address