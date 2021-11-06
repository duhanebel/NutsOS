[BITS 32]

section .asm

global paging_load_directory
global enable_paging

; To load the paging directory all is needed is to set CR3 to the address of the page directory
paging_load_directory:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]
    mov cr3, eax
    pop ebp
    ret

; Enabling paging is actually very simple. 
; All that is needed is to set the paging (PG) bit of CR0. 
; Note: setting the paging flag when the protection flag is clear causes a general-protection exception.
enable_paging:
    push ebp
    mov ebp, esp
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    pop ebp
    ret