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
mov cx, 0x0002
mov dh, 0x00
int 0x13

; save a known value to the beginning of the segment
mov [0], word 0xcafe

; jump to the COM file
jmp 0x1000:0x100

; boot-loader designator
times 510 - ($ - $$) db 0
dw 0b10101010_01010101
