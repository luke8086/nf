;
; Copyright (c) 2019 luke8086.
; Distributed under the terms of GPL-2 License.
;

;
; x86/nf_strt.asm - startup code
;

[cpu 8086]

section _TEXT class=CODE

resb 0x100
..start:

; jump to the C code
extern main
    jmp main

section _DATA class=DATA
section _BSS class=BSS
section _BSSEND class=BSSEND

; free memory begins at the end of BSS
global nf_heap_start
nf_heap_start:

group DGROUP _TEXT _DATA _BSS _BSSEND
