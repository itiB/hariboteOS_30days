[CPU 486]
[BITS 32]

    GLOBAL      api_end

section .text

api_end:       ; void api_end(void)
    MOV     EDX, 4
    INT     0x40