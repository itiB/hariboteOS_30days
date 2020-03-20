[CPU 486]
[BITS 32]

    GLOBAL      __alloca

section .text

__alloca:
    ADD     EAX, -4
    SUB     ESP, EAX
    JMP     DWORD [ESP + EAX] ; RETの代わり
