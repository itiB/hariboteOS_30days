; haribote-os

    ORG     0xc200      ; 0xc200 <- 0x8000 + 0x4200

    MOV     AL,0x13     ; VGAグラフィックスカラー, 320x200x8bitカラー
    MOV     AH,0x00
    INT     0x10

fin:
    HLT
    JMP     fin