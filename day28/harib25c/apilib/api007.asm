[CPU 486]
[BITS 32]

    GLOBAL      api_initmalloc

section .text

api_initmalloc: ; void api_initmalloc(void);
    PUSH    EBX
    MOV     EDX, 8
    MOV     EBX, [CS:0x0020]    ; malloc領域の番地
    MOV     EAX, EBX
    ADD     EAX, 32 * 1024      ; 32KBを足す
    MOV     ECX, [CS:0x0000]    ; データセグメントの大きさ
    SUB     ECX, EAX
    INT     0x40
    POP     EBX
    RET