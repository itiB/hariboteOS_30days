[CPU 486]
[BITS 32]

    GLOBAL      api_getlang

section .text

api_getlang:    ; int api_getlang(void);
    MOV     EDX, 27
    INT     0x40
    RET
