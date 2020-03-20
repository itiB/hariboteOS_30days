[CPU 486]
[BITS 32]

    GLOBAL      api_fsize

section .text

api_fsize:    ; void api_fsize(int fhandle, int mode);
    PUSH    EBX
    MOV     EDX, 24
    MOV     EAX, [ESP + 4]  ; fhandle
    MOV     ECX, [ESP + 8]  ; mode
    INT     0x40
    RET
