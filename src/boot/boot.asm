; the bios will start executing our boot loader from 0x7c00 so we need
; to map all the code starting there
ORG 0x7c00
BITS 16 ; i386 starts in real mode, which is 16bit

CODE_SEG equ gdt_code - gdt_start ; 0x08
DATA_SEG equ gdt_data - gdt_start ; 0x10

; boot sector descriptor - see https://wiki.osdev.org/FAT#Boot_Record
jmp short start ; Jump pass the boot sector descriptor
nop
OEMIdentifier           db 'MSWIN4.1' ; OEM identifier. The first 8 Bytes (3 - 10) is the version of DOS being used. 
BytesPerSector          dw 0x200 ; The number of Bytes per sector (remember, all numbers are in the little-endian format).
SectorsPerCluster       db 0x80 ; Number of sectors per cluster.
ReservedSectors         dw 200 ; Number of reserved sectors. The boot record sectors are included in this value.
FATCopies               db 0x02 ; Number of File Allocation Tables (FAT's) on the storage media.
RootDirEntries          dw 0x40 ; Number of directory entries (must be set so that the root directory occupies entire sectors).
NumSectors              dw 0x00 ; The total sectors in the logical volume. If this value is 0, it means there are more than 65535 sectors in the volume, and the actual count is stored in the Large Sector Count entry at 0x20.
MediaType               db 0xF8 ; This Byte indicates the media descriptor type (F8 = fixed disk)
SectorsPerFat           dw 0x100 ; Number of sectors per FAT. FAT12/FAT16 only.
SectorsPerTrack         dw 0x20 ; Number of sectors per track - useless if the FS is moved
NumberOfHeads           dw 0x40 ; Number of heads or sides on the storage media - useless if the FS is moved.
HiddenSectors           dd 0x00 ; Number of hidden sectors. 
SectorsBig              dd 0x773594 ; Large sector count. This field is set if there are more than 65535 sectors in the volume, resulting in a value which does not fit in the Number of Sectors entry at 0x13.

; Extended BPB (Dos 4.0)
DriveNumber             db 0x80 ; Drive number - 0x80 for hard disks - useless if the FS is moved
WinNTBit                db 0x00 ; Flags in Windows NT. Reserved otherwise.
Signature               db 0x29 ; Signature (must be 0x28 or 0x29).
VolumeID                dd 0xD105 ; VolumeID 'Serial' number. Used for tracking volumes between computers. 
VolumeIDString          db 'NUTSOS BOOT' ; Volume label string. Exactly 11 bytes - This field is padded with spaces.
SystemIDString          db 'FAT16   ' ; System identifier string. Exactly 8 bytes - This field is a string representation of the FAT file system type. It is padded with spaces.

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
    db 11001111b ; High 4 bit flags and the low 4 bit segment 16-19 bits
    db 0        ; Base 24-31 bits

; data segment - offset 0x10, size: 6bytes
gdt_data:      ; DS, SS, ES, FS, GS
    dw 0xffff ; Segment limit first 0-15 bits
    dw 0      ; Base first 0-15 bits
    db 0      ; Base 16-23 bits
    db 0x92   ; Access byte - r/w code, kernel privilege
    db 11001111b ; High 4 bit flags and the low 4 bit segment 16-19 bits
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
    mov edi, 0x00100000
    call ata_lba_read ; read the kernel from disk
    jmp CODE_SEG:0x00100000 ; jump to the kernel first instruction

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
