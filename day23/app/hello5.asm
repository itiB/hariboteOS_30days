[CPU 486]
[BITS 32]

    GLOBAL  HariMain

section .text

HariMain:
    MOV     EDX, 2
    MOV     EBX, msg
    INT     0x40
    MOV     EDX, 4
    INT     0x40

section .data
msg:
    DB "hello, world in asm!!", 0x0a, 0