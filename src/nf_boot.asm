;
; Copyright (c) 2019 luke8086.
; Distributed under the terms of GPL-2 License.
;

;
; x86/nf_boot.asm - minimal bootloader for USB disks (no error handling)
;

[org 0x7c00]
[cpu 8086]

TARGET_SEGMENT  equ 0x1000
TARGET_OFFSET   equ 0x100
SECTOR_COUNT    equ 48


    ; Setup segments and stack
    cli
    mov ax, TARGET_SEGMENT
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0xffff
    sti

    ; Preserve disk number
    mov [cs:disk], dl

    ; Print intro string using BIOS teletype output
    mov si, intro
.intro_loop:
    mov al, [cs:si]
    or al, al
    jz .intro_done
    mov ah, 0x0e
    xor bx, bx
    int 0x10
    inc si
    jmp .intro_loop
.intro_done:


    ; Retrieve sectors per track
    push es
    mov ah, 0x08
    mov dl, [cs:disk]
    int 0x13
    and cl, 0x3f
    mov [cs:spt], cl
    pop es

    ; Read data from floppy, one sector at a time, ignoring errors
    mov si, SECTOR_COUNT
    mov bx, TARGET_OFFSET
    xor ch, ch              ; Cylinder 0
    mov dl, [cs:disk]       ; Disk number
    xor dh, dh              ; Head 0
    mov cl, 3               ; Start sector (1-indexed)

.read_loop:
    ; Read one sector
    mov ax, 0x0201
    int 0x13

    call print_dot

    ; Advance target buffer
    add bx, 512

    ; Advance sector number
    inc cl
    cmp cl, [cs:spt]
    jbe .read_next

    ; Wrap to sector 1 and flip head
    mov cl, 1
    xor dh, 1

    ; If head is 0, advance cylinder
    jnz .read_next
    inc ch

.read_next:
    dec si
    jnz .read_loop


    ; Save a known value to the beginning of the segment
    mov [0], word 0xcafe


    ; Jump to the COM file
    jmp TARGET_SEGMENT:TARGET_OFFSET


; Print a debugging dot to the screen
print_dot:
    push ax
    push bx
    push cx
    push dx

    mov ah, 0x0e
    mov al, '.'
    xor bx, bx
    mov cx, 1
    int 0x10

    pop dx
    pop cx
    pop bx
    pop ax

    ret


; Sectors per track (default 18, auto-detected at boot)
spt: db 18

; Disk number
disk: db 0

; Intro text
intro: db 0x0d, 0x0a, "Booting NF [github.com/luke8086/nf]...", 0x00

; MBR partition table with a single bootable partition
times 0x1be - ($ - $$) db 0
db 0x80, 0x00, 0x02, 0x00
db 0x01, 0x00, 0x3f, 0x00
dd 0x01, 0x7f

; Boot-loader designator
times 0x1fe - ($ - $$) db 0
dw 0b10101010_01010101
