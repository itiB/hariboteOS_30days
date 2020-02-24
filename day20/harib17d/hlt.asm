; BITS 32

    MOV     AL, 'H'     ; ALレジスタに文字を入れる
    CALL    2*8:0xc67
    MOV     AL, 'e'
    CALL    2*8:0xc67
    MOV     AL, 'l'
    CALL    2*8:0xc67
    MOV     AL, 'l'
    CALL    2*8:0xc67
    MOV     AL, 'o'
    CALL    2*8:0xc67
    RETF

fin:
    HLT
    JMP fin