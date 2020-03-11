[CPU 486]
[BITS 32]

    GLOBAL      api_free

section .text

api_free:       ; void api_free(char *addr, int size);
    PUSH    EBX
    MOV     EDX, 10
    MOV     EBX, [CS:0x0020]
    MOV     EAX, [ESP +  8]     ; addr
    MOV     ECX, [ESP + 12]     ; size
    INT     0x40
    POP     EBX
    RET