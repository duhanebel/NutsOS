; the bios will start executing our boot loader from 0x7c00 so we need
; to map all the code starting there
ORG 0x7c00
BITS 16 ; i386 starts in real mode, which is 16bit

CODE_SEG equ gdt_code - gdt_start ; 0x08
DATA_SEG equ gdt_data - gdt_start ; 0x10

_start:
    jmp short start ; Jump pass the boot sector descriptor
    nop

; boot sector descriptor
 times 33 db 0

start:
    jmp 0:setup_segments

setup_segments:
    cli ; Clear Interrupts
    mov ax, 0x00
    mov ds, ax ; set data, extra and stack segments to 0
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00 ; setup the stack
    sti ; Enables Interrupts

.load_protected:
    cli
    lgdt[gdt_descriptor] ; load the GDT descriptor
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax ; Turn on protected mode by flipping the first bit of cr0
    jmp CODE_SEG:load32 ; load 0x08 on CS (code segment in the gdt, kernel access) and jump to 32bit code

; GDT - we're going to set up 2 overlapping segments as we don't care about segmentation
; as we'll use paging
gdt_start:
; first entry is always NULL
gdt_null:
    dd 0x0
    dd 0x0

; code segmen - offset 0x8, size: 6bytes
gdt_code:     ; CS SHOULD POINT TO THIS
    dw 0xffff ; Segment limit first 0-15 bits
    dw 0      ; Base first 0-15 bits
    db 0      ; Base 16-23 bits
    db 0x9a   ; Access byte - executable code, kernel privilege
    db 11001111b ; High 4 bit flags and the low 4 bit flags
    db 0        ; Base 24-31 bits

; data segment - offset 0x10, size: 6bytes
gdt_data:      ; DS, SS, ES, FS, GS
    dw 0xffff ; Segment limit first 0-15 bits
    dw 0      ; Base first 0-15 bits
    db 0      ; Base 16-23 bits
    db 0x92   ; Access byte - r/w code, kernel privilege
    db 11001111b ; High 4 bit flags and the low 4 bit flags
    db 0        ; Base 24-31 bits

; end of the entries
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start-1 ; length 
    dd gdt_start ; address of the first GDT entry

[BITS 32] ; switch to 32bit mode
load32:
    mov eax, 1
    mov ecx, 100 
    mov edi, 0x0100000
    call ata_lba_read ; read the kernel from disk
    jmp CODE_SEG:0x0100000 ; jump to the kernel first instruction

; load data from disk to ram
; eax - start sector
; ecx - amount of sectors
; edi - destination in ram
ata_lba_read:
    mov ebx, eax ; Backup the LBA
    ; Send the highest 8 bits of the lba to hard disk controller
    shr eax, 24
    or eax, 0xE0 ; Select the  master drive
    mov dx, 0x1F6
    out dx, al
    ; Finished sending the highest 8 bits of the lba

    ; Send the total sectors to read
    mov eax, ecx
    mov dx, 0x1F2
    out dx, al
    ; Finished sending the total sectors to read

    ; Send more bits of the LBA
    mov eax, ebx ; Restore the backup LBA
    mov dx, 0x1F3
    out dx, al
    ; Finished sending more bits of the LBA

    ; Send more bits of the LBA
    mov dx, 0x1F4
    mov eax, ebx ; Restore the backup LBA
    shr eax, 8
    out dx, al
    ; Finished sending more bits of the LBA

    ; Send upper 16 bits of the LBA
    mov dx, 0x1F5
    mov eax, ebx ; Restore the backup LBA
    shr eax, 16
    out dx, al
    ; Finished sending upper 16 bits of the LBA

    mov dx, 0x1F7
    mov al, 0x20
    out dx, al

    ; Read all sectors into memory
.next_sector:
    push ecx

; Checking if we need to read
.try_again:
    mov dx, 0x1f7
    in al, dx
    test al, 8
    jz .try_again

; We need to read 256 words at a time
    mov ecx, 256
    mov dx, 0x1F0
    rep insw
    pop ecx
    loop .next_sector
    ; End of reading sectors into memory
    ret

times 510-($ - $$) db 0 ; add padding for the remaining of the bits in the sector
dw 0xAA55 ; magic number to identify as a bootloader sector (must be the last dw of the first sector)
