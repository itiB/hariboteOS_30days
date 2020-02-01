;;  hello-os
;;  TAB=4

    ORG     0x7c00              ; メモリの読み込み位置を指定

;; FAT12フォーマットフロッピーディスク用の記述

    JMP     entry
    DB      0x90
    DB      "HELLOIPL"          ; ブートセクタの名前
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

;; ディスクを読む day03
    MOV     AX,0x0820
    MOV     ES,AX
    MOV     CH,0        ; シリンダ0
    MOV     DH,0        ; ヘッドセット0
    MOV     CL,2        ; セクタ2

    MOV     AH,0x02     ; AH=0x02 : ディスク読み込み
    MOV     AL,1        ; 1セクタ
    MOV     BX,0
    MOV     DL,0x00     ; Aドライブ
    INT     0x13        ; ディスクBIOSの呼び出し
    JC      error
fin:
    HLT                     ; 何かあるまでCPUを停止
    JMP     fin             ; 無限ループ
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
msg:
    DB      0x0a, 0x0a
    DB      "hello,worlds"
    DB      0x0a            ; 改行
    DB      0

    RESB    0x7dfe-0x7c00-($-$$)    ;; 現在から0x1fdまでを0で埋める

    DB      0x55, 0xaa
