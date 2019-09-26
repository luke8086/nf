;
; Copyright (c) 2019 luke8086.
; Distributed under the terms of GPL-2 License.
;

;
; x86/nf_boot.asm - minimal bootloader for USB disks
;

; point segments to 0x10000-0x1ffff
cli
mov ax, 0x1000
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0xffff
sti

; read 72 sectors from disk to 0x10100, dl is already set by BIOS
mov ax, 0x0248
mov bx, 0x0100
mov cx, 0x0003
mov dh, 0x00
int 0x13

; save a known value to the beginning of the segment
mov [0], word 0xcafe

; jump to the COM file
jmp 0x1000:0x100

; MBR partition table with a single bootable partition
times 0x1be - ($ - $$) db 0
db 0x80, 0x00, 0x02, 0x00
db 0x01, 0x00, 0x3f, 0x00
dd 0x01, 0x7f

; boot-loader designator
times 0x1fe - ($ - $$) db 0
dw 0b10101010_01010101
