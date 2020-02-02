; nasmfunc

[CPU    486 ]       ; 486命令まで使う宣言
[BITS   32 ]        ; 32ビットモードで機械語を作る

    GLOBAL  io_hlt
    GLOBAL  io_cli, io_sti, io_stihlt
    GLOBAL  io_in8, io_in16, io_in32
    GLOBAL  io_out8, io_out16, io_out32
    GLOBAL  io_load_eflags, io_store_eflags

section .text           ; オブジェクトファイルではこれを書いてからプログラムを書く

io_hlt:         ; void io_hlt(void);
    HLT
    RET         ;   return;

io_cli:         ; void io_cli(void);
    CLI         ; 割り込みフラグを0にする(clear interrupt flag)
    RET

io_sti:         ; void io_sti(void);
    STI         ; 割り込みフラグを1にする(set interrupt flag)
    RET

io_stihlt:      ; void io_stihlt(void);
    STI
    HLT
    RET

io_in8:         ; int io_in8(int port);
    MOV     EDX, [ESP + 4]      ; port
    MOV     EAX, 0
    IN      AL, DX              ; 信号を受け取る命令
    RET

io_in16:        ; int io_in16(int port);
    MOV     EDX, [ESP + 4]      ; port
    MOV     EAX, 0
    IN      AX, DX              ; 信号を受け取る命令
    RET

io_in32:        ; int io_in32(int port);
    MOV     EDX, [ESP + 4]      ; port
    MOV     EAX, 0
    IN      EAX, DX             ; 信号を受け取る命令
    RET

io_out8:        ; void io_in8(int port, int data);
    MOV     EDX, [ESP + 4]      ; port
    MOV     EAX, [ESP + 8]      ; data
    OUT     DX, AL              ; 信号を送る命令
    RET

io_out16:       ; void io_in16(int port, int data);
    MOV     EDX, [ESP + 4]      ; port
    MOV     EAX, [ESP + 8]      ; data
    OUT     DX, AX              ; 信号を送る命令
    RET

io_out32:       ; void io_in32(int port, int data);
    MOV     EDX, [ESP + 4]      ; port
    MOV     EAX, [ESP + 8]      ; data
    OUT     DX, EAX             ; 信号を送る命令
    RET

io_load_eflags: ; int io_load_eflag(void);
    PUSHFD                      ; PUSH EFLAGS + DOBULE WORD
    POP     EAX                 ; スタックに入っていたeflagsをEAXに入れて返す
    RET

io_store_eflags: ; void io_store_eflag(int eflags);
    MOV     EAX, [ESP + 4]
    PUSH    EAX                 ; 入っている値が返り値として用いられる
    POPFD                       ; POP EFLAGS + DOBULE WORD
    RET