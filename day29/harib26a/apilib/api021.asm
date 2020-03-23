[CPU 486]
[BITS 32]

    GLOBAL      api_fopen

section .text

api_fopen:    ; int api_fopen(char *fname);
    PUSH    EBX
    MOV     EDX, 21
    MOV     EBX, [ESP + 8]  ; fname
    INT     0x40
    POP     EBX
    RET
