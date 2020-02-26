; BITS 32
    MOV     AL, 'H'     ; ALレジスタに文字を入れる
    INT     0x40
    MOV     AL, 'e'
    INT     0x40
    MOV     AL, 'l'
    INT     0x40
    MOV     AL, 'l'
    INT     0x40
    MOV     AL, 'o'
    INT     0x40
    RETF

fin:
    HLT
    JMP fin