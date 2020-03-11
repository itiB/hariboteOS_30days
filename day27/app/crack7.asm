[BITS 32]
[CPU 486]

    GLOBAL  HariMain

section .text

HariMain:
    MOV     AX, 1005 * 8
    MOV     DS, AX
    CMP     DWORD [DS:0x0004], 'Hari'
    JNE     fin     ; アプリではないようなのでなにもしない

    MOV     ECX, [DS:0x0000]    ; データセグメントの大きさを読み取る
    MOV     AX, 2005 * 8
    MOV     DS, AX

crackloop:      ; 123でうめつくす
    ADD     ECX, -1
    MOV     BYTE [DS:ECX], 123
    CMP     ECX, 0
    JNE     crackloop

fin:
    MOV     EDX, 4
    INT     0x40