; nasmfunc

    GLOBAL  io_hlt

section .text         ;オブジェクトファイルではこれを書いてからプログラムを書く

io_hlt:                ;void io_hlt(void);
    HLT
    RET