;;  hello-os
;;  TAB=4

;; FAT12フォーマットフロッピーディスク用の記述

DB      0xeb, 0x4e, 0x90
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

DB      0xb8, 0x00, 0x00, 0x8e, 0xd0, 0xbc, 0x00, 0x7c
DB      0x8e, 0xd8, 0x8e, 0xc0, 0xbe, 0x74, 0x7c, 0x8a
DB      0x04, 0x83, 0xc6, 0x01, 0x3c, 0x00, 0x74, 0x09
DB      0xb4, 0x0e, 0xbb, 0x0f, 0x00, 0xcd, 0x10, 0xeb
DB      0xee, 0xf4, 0xeb, 0xfd

;; メッセージ部分

DB      0x0a, 0x0a
DB      "Hello, worldtest"
DB      0x0a
DB      0

RESB    0x1fe-($-$$)

DB      0x55, 0xaa

;; ブートセクタ以外

DB      0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
RESB    4600
DB      0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
RESB    1469432