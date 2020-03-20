[CPU 486]
[BITS 32]

    GLOBAL      api_fread

section .text

api_fread:    ; void api_fread(char *buf, int mxsize, int fhandle);
    PUSH    EBX
    MOV     EDX, 25
    MOV     EAX, [ESP + 16] ; fhandle
    MOV     ECX, [ESP + 12] ; maxsize
    MOV     EBX, [ESP + 8] ; buf
    INT     0x40
    POP     EBX
    RET
