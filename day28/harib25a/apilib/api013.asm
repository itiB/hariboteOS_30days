[CPU 486]
[BITS 32]

    GLOBAL      api_closewin

section .text

api_closewin:   ; void api_linewin(sht, eax, ecx, esi, edi, ebp);
    PUSH    EBX
    MOV     EDX, 14
    MOV     EBX, [ESP + 8] ; winl
    INT     0x40
    POP     EBX
    RET
