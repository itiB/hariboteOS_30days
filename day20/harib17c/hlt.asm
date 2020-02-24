; BITS 32

    MOV     AL, 'A'     ; ALレジスタに文字を入れる
    CALL    2*8:0xc67

fin:
    HLT
    JMP fin