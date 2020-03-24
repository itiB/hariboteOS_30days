;;  hello-os
;;  TAB=4

    ORG     0x7c00              ; メモリの読み込み位置を指定
    CYLS    EQU     20          ; #define CYLS = 10

;; FAT12フォーマットフロッピーディスク用の記述

    JMP     entry
    DB      0x90
    DB      "HARIBOTE"          ; ブートセクタの名前
    DW      512                 ; 1セクタの大きさ
    DB      1                   ; クラスタの大きさ
    DW      1                   ; FATの開始位置
    DB      2                   ; FATの個数
    DW      224                 ; ルートディレクトリ領域の大きさ
    DW      2880                ; ドライブの大きさ
    DB      0xf0                ; メディアのタイプ
    DW      9                   ; FAT領域の長さ
    DW      18                  ; 1トラック中のセクタ数
    DW      2                   ; ヘッドの数
    DD      0                   ; パーティションを使用していなければ0
    DD      2880                ; ドライブの大きさ
    DB      0, 0, 0x29          ; 固定値？
    DD      0xffffffff          ; ボリュームシリアル番号？
    DB      "HELLO-OS   "       ; ディスク名
    DB      "FAT12   "          ; フォーマット名
    RESB    18                  ; 18バイト空ける

;; プログラム本体
entry:
    MOV     AX,0            ; 代入処理 AX=0
    MOV     SS,AX
    MOV     SP,0x7c00
    MOV     DS,AX

; ディスクを読む
    MOV     AX,0x0820
    MOV     ES,AX
    MOV     CH,0        ; シリンダ0
    MOV     DH,0        ; ヘッドセット0
    MOV     CL,2        ; セクタ2
    MOV     BX, 18*2*CYLS-1 ; 読み込みたい合計セクタ数
    CALL    readfast
; 読み終わったらharibote.sysを実行
    MOV     BYTE [0x0ff0], CYLS  ; IPLがどこまで読んだのかをメモ
    JMP     0xc200

error:
    MOV     AX, 0
    MOV     ES, AX
    MOV     SI,msg

putloop:
    MOV     AL,[SI]
    ADD     SI,1
    CMP     AL,0
    JE      fin
    MOV     AH,0x0e         ; 1文字表示ファンクション
    MOV     BX,15           ; カラーコード
    INT     0x10            ; ビデオBIOS呼び出し
    JMP     putloop

fin:
    HLT                     ; 何かあるまでCPUを停止
    JMP     fin             ; 無限ループ

msg:
    DB      0x0a, 0x0a      ; 改行*2
    DB      "load error"
    DB      0x0a            ; 改行
    DB      0

readfast:
    ; ALを使ってまとめて読み出す
    ; ES: 読み込み番地
    ; CH: シリンダ
    ; DH: ヘッド
    ; CL: セクタ
    ; BX: 読み込みセクタ数

    MOV     AX, ES      ; < ESからALの最大値を計算 >
    SHL     AX, 3       ; = AXを32で割ってその結果をAHに入れた
    AND     AH, 0x7f    ; AHはAHを128で割ったあまり
    MOV     AL, 128
    SUB     AL, AH      ; AL = 128 - AH; 一番近い64KB境界まで最大何セクタ入るか

    MOV     AH, BL      ; < BXからALの最大値をAHに計算 >
    CMP     BH, 0       ; if (BH != 0) { AH = 18; }
    JE      .skip1
    MOV     AH, 18
.skip1:
    CMP     AL, AH      ; if (AL > AH) { AL = AH; }
    JBE     .skip2
    MOV     AL, AH
.skip2:
    MOV     AH, 19      ; < CLからALの最大値をAHに計算 >
    SUB     AH, CL      ; AH = 19 - CL;
    CMP     AL, AH      ; if (AL > AH) { AL = AB; }
    JBE     .skip3
    MOV     AL, AH
.skip3:
    PUSH    BX
    MOV     SI, 0       ; 失敗回数を数えるレジスタ

retry:
    MOV     AH,0x02     ; AH=0x02 : ディスク読み込み
    MOV     BX,0
    MOV     DL,0x00     ; Aドライブ
    PUSH    ES
    PUSH    DX
    PUSH    CX
    PUSH    AX
    INT     0x13        ; ディスクBIOSの呼び出し
    JNC     next        ; エラーが起きなければnextへ
    ADD     SI,1
    CMP     SI,5        ; SIと5を比較する
    JAE     error       ; SI >= 5ならばerror
    MOV     AH,0x00
    MOV     DL,0x00     ; Aドライブ
    INT     0x13        ; ドライブのリセット
    POP     AX
    POP     CX
    POP     DX
    POP     ES
    JMP     retry

next:
    POP     AX
    POP     CX
    POP     DX
    POP     BX          ; Esの内容をBXで受け取る
    SHR     BX, 5       ; BXを16バイト単位から512バイト単位へ
    MOV     AH, 0
    ADD     BX, AX      ; BX += AL;
    SHL     BX, 5       ; BXを512バイト単位から16バイト単位へ
    MOV     ES, BX      ; ES += AL * 0x20
    POP     BX
    SUB     BX, AX
    JZ      .ret
    ADD     CL, AL      ; CL += AL
    CMP     CL, 18      ; CLと18を比較
    JBE     readfast    ; CL <= 18ならばreadfast
    MOV     CL, 1
    ADD     DH, 1
    CMP     DH, 2
    JB      readfast    ; DH < 2だったらreadfastへ
    MOV     DH, 0
    ADD     CH, 1
    JMP     readfast
.ret:
    RET
    RESB    0x7dfe - 0x7c00 - ($ - $$)


; END CODE
    DB      0x55, 0xaa
