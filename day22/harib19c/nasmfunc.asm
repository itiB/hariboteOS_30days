; nasmfunc

[CPU   486 ]       ; 486命令まで使う宣言
[BITS   32 ]        ; 32ビットモードで機械語を作る

    GLOBAL  io_hlt
    GLOBAL  io_cli, io_sti, io_stihlt
    GLOBAL  io_in8, io_in16, io_in32
    GLOBAL  io_out8, io_out16, io_out32
    GLOBAL  io_load_eflags, io_store_eflags
    GLOBAL  load_gdtr, load_idtr
    GLOBAL  asm_inthandler21, asm_inthandler27, asm_inthandler2c
    GLOBAL  load_cr0, store_cr0
    GLOBAL  asm_inthandler20
    GLOBAL  load_tr, farjmp, farcall
    GLOBAL  asm_hrb_api, start_app, asm_end_app
    GLOBAL  asm_inthandler0c, asm_inthandler0d

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
    IN      EAX, DX
    RET

io_out8:        ; void io_in8(int port, int data);
    MOV     EDX, [ESP + 4]      ; port
    MOV     AL, [ESP + 8]      ; data
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

load_gdtr:  ; void load_gdtr(int limit, int addr);
    MOV     AX, [ESP + 4]       ; limitをあらわす16bit
    MOV     [ESP + 6], AX       ; [ESP + 8]に入っているアドレスと連結
    LGDT    [ESP + 6]           ; GDTRレジスタ(48bit)に代入
    RET

load_idtr:  ; void load_idtr(int limit, int addr);
    MOV     AX,[ESP + 4]
    MOV     [ESP + 6],AX
    LIDT    [ESP + 6]
    RET


; 割り込み終了後用のIRETD命令
    EXTERN  inthandler20, inthandler21, inthandler27, inthandler2c

asm_inthandler20:   ; IRQ0(タイマ割り込み)発生時に通知する
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     EAX, ESP
    PUSH    EAX
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    inthandler20
    POP     EAX
    POPAD
    POP     DS
    POP     ES
    IRETD

asm_inthandler21:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     EAX, ESP
    PUSH    EAX
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    inthandler21
    POP     EAX
    POPAD
    POP     DS
    POP     ES
    IRETD

asm_inthandler27:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     EAX, ESP
    PUSH    EAX
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    inthandler27
    POP     EAX
    POPAD
    POP     DS
    POP     ES
    IRETD

asm_inthandler2c:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     EAX, ESP
    PUSH    EAX
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    inthandler2c
    POP     EAX
    POPAD
    POP     DS
    POP     ES
    IRETD

load_cr0:   ; int load_cr0(void);
    MOV     EAX, CR0
    RET

store_cr0:  ; void store_cr0(int cr0);
    MOV     EAX, [ESP + 4]
    MOV     CR0, EAX
    RET


; マルチタスクするために

; TRレジスタの値を書き換える
load_tr:    ; void load_tr(int tr);
    LTR     [ESP + 4]   ; tr
    RET

farjmp:        ; void farjmp(int eip, int cs);
    JMP     FAR [ESP + 4]   ; eip = esp + 4
    RET                     ; cs  = esp + 8

farcall:  ; void farcall(int eip, int cs);
	CALL    FAR [ESP+4]     ; eip, cs
	RET

; 一文字表示API(console.c/cons_putchar())
    EXTERN cons_putchar

; 一文字表示API
asm_cons_putchar:
    STI                     ; 割り込みとして処理するため入力とかできなくなっているのを解除
    PUSHAD
    PUSH    1
    AND     EAX, 0xff       ; AHやEAXの上位を0にしてEAXに文字が入った状態にする
    PUSH    EAX
    PUSH    DWORD [0xfec]   ; メモリの内容を読み込んでその値をPush(console.c/console_task())
    CALL    cons_putchar
    ADD     ESP, 12
    POPAD
    IRETD                   ; 割り込みでAPIを処理するため戻るときはIRETD

; APIの処理を基本的にCで行う
    EXTERN  hrb_api

asm_hrb_api:
    STI
    PUSH    DS
    PUSH    ES
    PUSHAD                  ; 現状を保存するPUSH
    PUSHAD                  ; hrb_apiに渡すためのPUSH
    MOV     AX, SS
    MOV     DS, AX          ; OSのセグメントをDSとESにも入れる
    MOV     ES, AX
    CALL    hrb_api
    CMP     EAX, 0          ; EAXが0出ないならアプリを終了させる
    JNE     asm_end_app
    ADD     ESP, 32
    POPAD
    POP     ES
    POP     DS
    IRETD

asm_end_app:
; EAXはtss.esp0の番地をあらわしている
    MOV     ESP, [EAX]
    MOV     DWORD [EAX + 4], 0
    POPAD
    RET                     ; cmd_appへ帰る

start_app:  ; void start_app(int eip, int cs, int esp, int ds, int *tss_esp);
    PUSHAD                  ; 32bitレジスタに保存する
    MOV     EAX, [ESP + 36] ; アプリ用のEIP
    MOV     ECX, [ESP + 40] ; アプリ用のCS
    MOV     EDX, [ESP + 44] ; アプリ用のESP
    MOV     EBX, [ESP + 48] ; アプリ用のDS/SS
    MOV     EBP, [ESP + 52] ; tss.esp0の番地を伝える
    MOV     [EBP    ], ESP  ; OS用のESP, 戻ってきたときに使うために保存
    MOV     [EBP + 4], SS
    MOV     ES, BX
    MOV     DS, BX
    MOV     FS, BX
    MOV     GS, BX
; RFTFでアプリに生かせるためスタックを調整
    OR      ECX, 3          ; アプリ用のセグメント番号に3をOR(おまじない)
    OR      EBX, 3          ; アプリ用のセグメント番号に3をOR(おまじない)
    PUSH    EBX             ; アプリのSS
    PUSH    EDX             ; アプリのESP
    PUSH    ECX             ; アプリのCS
    PUSH    EAX             ; アプリのEIP
    RETF                    ; アプリの呼び出し！
; アプリが終了してもここには返ってこない

EXTERN inthandler0d, inthandler0c

; スタック例外
asm_inthandler0c:
    STI
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     EAX, ESP
    PUSH    EAX
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    inthandler0c
    CMP     EAX, 0
    JNE     asm_end_app
    POP     EAX
    POPAD
    POP     DS
    POP     ES
    ADD     ESP, 4          ; INT 0x0dではこれが必要
    IRETD

; 異常終了処理用
asm_inthandler0d:
    STI
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     EAX, ESP
    PUSH    EAX
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    inthandler0d
    CMP     EAX, 0
    JNE     asm_end_app
    POP     EAX
    POPAD
    POP     DS
    POP     ES
    ADD     ESP, 4          ; INT 0x0dではこれが必要
    IRETD