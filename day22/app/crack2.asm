[CPU 486]
[BITS 32]

    MOV     EAX, 1 * 8          ; OS用のセグメント番号
    MOV     DS, AX              ; これをDSに入れる
    MOV     BYTE [0x102600], 0
    MOV     EDX, 4
    INT     0x40