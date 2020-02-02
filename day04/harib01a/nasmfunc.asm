; nasmfunc
section .text           ; オブジェクトファイルではこれを書いてからプログラムを書く
    GLOBAL  io_hlt
    GLOBAL  write_mem8

io_hlt:                 ; void io_hlt(void);
    HLT
    RET

; 直接メモリを指定して書き込む関数を作る
;   Cから自由に触れるレジスタはEAX, ECX, EDX
;   他は参照できるけどいじれない
write_mem8:                 ; void write_mem8(int addr, int data);
    MOV     ECX, [ESP+4]  ; [ESP + 4]にaddrが入っているのでそれを読み込み
    MOV     AL, [ESP+8]   ; [ESP + 8]に入っているdata読み込み
    MOV     [ECX], AL
    RET