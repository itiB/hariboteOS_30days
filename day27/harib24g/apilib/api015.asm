[CPU 486]
[BITS 32]

    GLOBAL      api_alloctimer

section .text

api_alloctimer: ; int api_alloctimer(void);
    MOV     EDX, 16
    INT     0x40
    RET