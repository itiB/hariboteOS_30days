#include "bootpack.h"

/*
 * PICの初期化(Programmable interrupt controller)
 *  8つある割り込み信号を一つの信号にまとめる
 *  2つ目のPICがあって合計15個の割り込み信号が使える
 */

void init_pic(void) {
    // IMR 割り込み目隠しレジスタ 割り込み受付を制御
    // ICW 初期化制御データ
    //  1-4が存在，合計4byte
    //  1, 4 配線とかとかの設定
    //  3 マスタスレーブの設定
    //  2 OSごとの設定，IRQをどの割り込み番号として通知するか決める

    io_out8(PIC0_IMR,    0xff); // すべての割り込みを受け付けない
    io_out8(PIC1_IMR,    0xff); // すべての割り込みを受け付けない

    io_out8(PIC0_ICW1,   0x11); // エッジトリガモード
    io_out8(PIC0_ICW2,   0x20); // IRQ0-7はINT20-27で受け取る
    io_out8(PIC0_ICW3, 1 << 2); // PIC1はIRQ2で接続
    io_out8(PIC0_ICW4,   0x01); // ノンバッファモード

    io_out8(PIC1_ICW1,   0x11); // エッジトリガモード
    io_out8(PIC1_ICW2,   0x28); // IRQ8-15はINT28-2fで受け取る
    io_out8(PIC1_ICW3,      2); // PIC1はIRQ2で接続
    io_out8(PIC1_ICW4,   0x01); // ノンバッファモード

    io_out8(PIC0_IMR,    0xfb); // 11111011 PIC1以外はすべて停止
    io_out8(PIC1_IMR,    0xff); // すべての割り込みを受け付けない

    return;
}

// 割り込みハンドラの作成


// PS/2キーボードからの割り込み
void inthandler21(int *esp) {
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    unsigned char data, s[4];

    // 動きを見やすくするために画面を初期化 
    // init_screen(binfo -> vram, binfo -> scrnx, binfo -> scrny);

    io_out8(PIC0_OCW2, 0x61);   // IRQ-01受付完了をPICに通知, 再度監視刺せる
    data = io_in8(PORT_KEYDAT); // dataを取ってくる

    if (keybuf.next < 32) {
        keybuf.data[keybuf.next] = data;
        keybuf.next++;
    }

    return;
}

// PS/2キーボードからの割り込み
void inthandler2c(int *esp) {
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 -1, 15);
    putfont8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, "INT 2C(IRQ-12) PS/2 mouse");
    for (;;) {
        io_hlt();
    }
}

void inthandler27(int *esp) {
    io_out8(PIC0_OCW2, 0x67);
    return;
}