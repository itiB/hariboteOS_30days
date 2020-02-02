; nasmfunc
section .text           ; オブジェクトファイルではこれを書いてからプログラムを書く
    GLOBAL  io_hlt

io_hlt:                 ; void io_hlt(void);
    HLT
    RET
