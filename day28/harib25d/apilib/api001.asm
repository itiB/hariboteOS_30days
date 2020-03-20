[CPU 486]
[BITS 32]

    GLOBAL      api_putchar

section .text

api_putchar:    ; void api_putchar(int c)
    MOV     EDX, 1
    MOV     AL, [ESP + 4]   ; c
    INT     0x40
    RET
