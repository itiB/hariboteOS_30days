[CPU    486 ]       ; 486命令まで使う宣言
[BITS   32 ]        ; 32ビットモードで機械語を作る

    MOV     EDX, 2
    MOV     EBX, msg
    INT     0x40
    MOV     EDX, 4
    INT     0x40

msg:
    DB      "hello", 0