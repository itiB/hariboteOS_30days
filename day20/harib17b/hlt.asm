; BITS 32

    MOV     AL, 'A'     ; ALレジスタに文字を入れる
    CALL    0xc62       ; cons_putcharの番地

fin:
    HLT
    JMP fin