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
    GLOBAL  asm_hrb_api, start_app

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

asm_inthandler21:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     AX,SS
    CMP     AX, 1 * 8
    JNE     .from_app
; OSが動いているときに割り込まれたら今まで通り
    MOV     EAX,ESP
    PUSH    SS          ; 割り込みの際のSSを保存
    PUSH    EAX         ; 割り込みの際のESPを保存
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    inthandler21
    ADD     ESP, 8
    POPAD
    POP     DS
    POP     ES
    IRETD
.from_app
; appが動いているときに割り込まれた
    MOV     EAX, 1 * 8
    MOV     DS, AX          ; とりあえずDSをOS用にする
    MOV     ECX, [0xfe4]    ; OS用のESP
    ADD     ECX, -8
    MOV     [ECX + 4], SS   ; 割り込み発生時のSSを保存
    MOV     [ECX    ], ESP  ; 割り込み発生時のESPを保存
    MOV     SS, AX
    MOV     ES, AX
    MOV     ESP, ECX
    CALL    inthandler21
    POP     ECX
    POP     EAX
    MOV     SS, AX          ; SSをアプリ用に戻す
    MOV     ESP, ECX        ; ESPをアプリ用に戻す
    POPAD
    POP     DS
    POP     ES
    IRETD

asm_inthandler27:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     AX,SS
    CMP     AX, 1 * 8
    JNE     .from_app
; OSが動いているときに割り込まれたら今まで通り
    MOV     EAX,ESP
    PUSH    SS          ; 割り込みの際のSSを保存
    PUSH    EAX         ; 割り込みの際のESPを保存
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    inthandler27
    ADD     ESP, 8
    POPAD
    POP     DS
    POP     ES
    IRETD
.from_app
; appが動いているときに割り込まれた
    MOV     EAX, 1 * 8
    MOV     DS, AX          ; とりあえずDSをOS用にする
    MOV     ECX, [0xfe4]    ; OS用のESP
    ADD     ECX, -8
    MOV     [ECX + 4], SS   ; 割り込み発生時のSSを保存
    MOV     [ECX    ], ESP  ; 割り込み発生時のESPを保存
    MOV     SS, AX
    MOV     ES, AX
    MOV     ESP, ECX
    CALL    inthandler27
    POP     ECX
    POP     EAX
    MOV     SS, AX          ; SSをアプリ用に戻す
    MOV     ESP, ECX        ; ESPをアプリ用に戻す
    POPAD
    POP     DS
    POP     ES
    IRETD

asm_inthandler2c:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     AX,SS
    CMP     AX, 1 * 8
    JNE     .from_app
; OSが動いているときに割り込まれたら今まで通り
    MOV     EAX,ESP
    PUSH    SS          ; 割り込みの際のSSを保存
    PUSH    EAX         ; 割り込みの際のESPを保存
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    inthandler2c
    ADD     ESP, 8
    POPAD
    POP     DS
    POP     ES
    IRETD
.from_app
; appが動いているときに割り込まれた
    MOV     EAX, 1 * 8
    MOV     DS, AX          ; とりあえずDSをOS用にする
    MOV     ECX, [0xfe4]    ; OS用のESP
    ADD     ECX, -8
    MOV     [ECX + 4], SS   ; 割り込み発生時のSSを保存
    MOV     [ECX    ], ESP  ; 割り込み発生時のESPを保存
    MOV     SS, AX
    MOV     ES, AX
    MOV     ESP, ECX
    CALL    inthandler2c
    POP     ECX
    POP     EAX
    MOV     SS, AX          ; SSをアプリ用に戻す
    MOV     ESP, ECX        ; ESPをアプリ用に戻す
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

asm_inthandler20:   ; IRQ0(タイマ割り込み)発生時に通知する
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     AX,SS
    CMP     AX, 1 * 8
    JNE     .from_app
; OSが動いているときに割り込まれたら今まで通り
    MOV     EAX,ESP
    PUSH    SS          ; 割り込みの際のSSを保存
    PUSH    EAX         ; 割り込みの際のESPを保存
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    inthandler20
    ADD     ESP, 8
    POPAD
    POP     DS
    POP     ES
    IRETD
.from_app
; appが動いているときに割り込まれた
    MOV     EAX, 1 * 8
    MOV     DS, AX          ; とりあえずDSをOS用にする
    MOV     ECX, [0xfe4]    ; OS用のESP
    ADD     ECX, -8
    MOV     [ECX + 4], SS   ; 割り込み発生時のSSを保存
    MOV     [ECX    ], ESP  ; 割り込み発生時のESPを保存
    MOV     SS, AX
    MOV     ES, AX
    MOV     ESP, ECX
    CALL    inthandler20
    POP     ECX
    POP     EAX
    MOV     SS, AX          ; SSをアプリ用に戻す
    MOV     ESP, ECX        ; ESPをアプリ用に戻す
    POPAD
    POP     DS
    POP     ES
    IRETD

; マルチタスクするために

; TRレジスタの値を書き換える
load_tr:    ; void load_tr(int tr);
    LTR     [ESP + 4]   ; tr
    RET

; farモードジャンプ
;taskswitch4:    ; void taskswitch4(void);
;    JMP     4*8:0
;    RET
;
;taskswitch3:    ; void taskswitch3(void);
;    JMP     3*8:0
;    RET

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
    PUSH    DS
    PUSH    ES
    PUSHAD                  ; アプリのスタックに保存するPUSH
    MOV     EAX, 1 * 8
    MOV     DS, AX          ; とりあえずDSをOS用に
    MOV     ECX, [0xfe4]    ; OSのESP
    ADD     ECX, -40
    MOV     [ECX + 32], ESP ; アプリのESPを保存
    MOV     [ECX + 36], SS  ; アプリのSSを保存

; PUSHADした値をシステムのスタックにコピー
    MOV     EDX, [ESP    ]
    MOV     EBX, [ESP + 4]
    MOV     [ECX    ], EDX      ; HRB_apiに渡すためにコピー
    MOV     [ECX + 4], EBX      ; HRB_apiに渡すためにコピー
    MOV     EDX, [ESP +  8]
    MOV     EBX, [ESP + 12]
    MOV     [ECX +  8], EDX     ; HRB_apiに渡すためにコピー
    MOV     [ECX + 12], EBX     ; HRB_apiに渡すためにコピー
    MOV     EDX, [ESP + 16]
    MOV     EBX, [ESP + 20]
    MOV     [ECX + 16], EDX     ; HRB_apiに渡すためにコピー
    MOV     [ECX + 20], EBX     ; HRB_apiに渡すためにコピー
    MOV     EDX, [ESP + 24]
    MOV     EBX, [ESP + 28]
    MOV     [ECX + 24], EDX     ; HRB_apiに渡すためにコピー
    MOV     [ECX + 28], EBX     ; HRB_apiに渡すためにコピー

    MOV     ES, AX              ; 残りのセグメントもOS用にする
    MOV     SS, AX
    MOV     ESP, ECX
    STI                         ; 割り込み許可

    CALL hrb_api

    MOV     ECX, [ESP + 32]     ; アプリのESPを戻す
    MOV     EAX, [ESP + 36]     ; アプリのESPを戻す
    CLI                         ; 割り込み禁止
    MOV     SS, AX
    MOV     ESP, ECX
    POPAD
    POP     ES
    POP     DS
    IRETD                   ; 割り込みでAPIを処理するため戻るときはIRETD

start_app:  ; void start_app(int eip, int cs, int esp, int ds);
    PUSHAD                  ; 32bitレジスタに保存する
    MOV     EAX, [ESP + 36] ; アプリ用のEIP
    MOV     ECX, [ESP + 40] ; アプリ用のCS
    MOV     EDX, [ESP + 44] ; アプリ用のESP
    MOV     EBX, [ESP + 48] ; アプリ用のDS/SS
    MOV     [0xfe4], ESP    ; OS用のESP, 戻ってきたときに使うために保存
    CLI                     ; 切り替え中に割り込みが来ないようにする
    MOV     ES, BX
    MOV     SS, BX
    MOV     DS, BX
    MOV     FS, BX
    MOV     GS, BX
    MOV     ESP, EDX
    STI                     ; 切り替え完了につき割り込みを許可
    PUSH    ECX             ; far-CALLのためにPUSH(cs)
    PUSH    EAX             ; far-CALLのためにPUSH(eip)
    CALL    FAR [ESP]       ; アプリの呼び出し

; アプリ終了後にここに戻ってくる
    MOV     EAX, 1 * 8      ; OS用のDS/SS
    CLI                     ; また切り替えのために割り込みを禁止
    MOV     ES, AX
    MOV     SS, AX
    MOV     DS, AX
    MOV     FS, AX
    MOV     GS, AX
    MOV     ESP, [0xfe4]
    STI                     ; 切り替え完了につき割り込み許可
    POPAD                   ; 保存していたレジスタを回復
    RET
