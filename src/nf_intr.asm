;
; Copyright (c) 2019 luke8086.
; Distributed under the terms of GPL-2 License.
;

;
; x86/nf_intr.asm - a routine for triggering software interrupts
;

section _TEXT class=CODE

; struct nf_regs { int ax, bx, cx, dx, bp, di, si, flags };
; void intr(int int_no, struct nf_regs *) { ... };
global nf_intr
nf_intr:
    ; create a new stack frame
    push bp
    mov bp, sp

    ; preserve non-volatile registers, as required by calling convention
    push si
    push di

    ; preserve bp, since some BIOS calls take it as an argument
    push bp

    ; inject the interrupt number to the INT instruction, since it only
    ; works with an immediate value
    mov ax, [bp+4]
    mov byte [intr_int+1], al

    ; load values from regs_t (except flags) to the actual registers
    mov si, [bp+6]
    mov ax, [si+0]
    mov bx, [si+2]
    mov cx, [si+4]
    mov dx, [si+6]
    mov bp, [si+8]
    mov di, [si+10]
    mov si, [si+12]

    ; trigger the interrupt. the number will be injected by the code above
intr_int:
    int 0

    ; restore previously preserved bp
    pop bp

    ; store [abcd]x registers back to regs_t
    mov si, [bp+6]
    mov [si+0], ax
    mov [si+2], bx
    mov [si+4], cx
    mov [si+6], dx

    ; store FLAGS to regs_t
    pushf
    pop word [si+14]

    ; restore previously preserved non-volatile registers
    pop di
    pop si

    ; destroy the stack frame and return
    pop bp
    ret

