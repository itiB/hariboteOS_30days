;;  hello-os
;;  TAB=4

    ORG     0x7c00              ; メモリの読み込み位置を指定
    CYLS    EQU     10          ; #define CYLS = 10

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

;; ディスクを読む
    MOV     AX,0x0820
    MOV     ES,AX
    MOV     CH,0        ; シリンダ0
    MOV     DH,0        ; ヘッドセット0
    MOV     CL,2        ; セクタ2

readloop:
    MOV     SI,0        ; 失敗数を数えるレジスタ(source index)

retry:
    MOV     AH,0x02     ; AH=0x02 : ディスク読み込み
    MOV     AL,1        ; 1セクタ
    MOV     BX,0
    MOV     DL,0x00     ; Aドライブ
    INT     0x13        ; ディスクBIOSの呼び出し
    JNC     next        ; エラーが起きなければnextへ
    ADD     SI,1
    CMP     SI,5        ; SIと5を比較する
    JAE     error       ; SI >= 5ならばerror
    MOV     AH,0x00
    MOV     DL,0x00     ; Aドライブ
    INT     0x13        ; ドライブのリセット
    JMP     retry

next:
    MOV     AX,ES       ; アドレスを0x200進める
    ADD     AX,0x0020
    MOV     ES,AX       ; ADD ES, 0x020の命令がないため2行使っている
    ADD     CL,1        ; CLに1をたす(セクタを進める)
    CMP     CL,18       ; CLと18を比較
    JBE     readloop    ; CL <= 18だったらreadloopに飛ぶ(条件ジャンプ)
    MOV     CL,1        ; 19セクタ目に突入しようとしたらシリンダを進める
    ADD     DH,1
    CMP     DH,2
    JB      readloop    ; DH < 2だったらreadloopへ
    MOV     DH,0
    ADD     CH,1
    CMP     CH,CYLS
    JB      readloop    ; CH < CYLSだったらreadloopへ
; ブートセクタの読み込みが終わったのでOS本体を実行する
    MOV     [0x0ff0], CH    ; IPLがどこまで読んだのかをメモ
    JMP     0xc200

error:
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
    DB      0x0a, 0x0a
    DB      "hello,worlds"
    DB      0x0a            ; 改行
    DB      0

    RESB    0x7dfe - 0x7c00 - ($ - $$)    ;; 現在から0x1fdまでを0で埋める

; END CODE
    DB      0x55, 0xaa
