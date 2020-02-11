#include "harib08a/bootpack.h"

// マウスの有効化
void enable_mouse(struct MOUSE_DEC *mdec) {
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);  // マウス使っていいよ～～～
    mdec->phase = 0;  // マウスの返事(0xfa)を持っている状態
    return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat) {
    if (mdec->phase == 0) {
        // マウスの0xfaを持っている
        if (dat == 0xfa) {
            mdec->phase = 1;
        }
        return 0;
    }
    if (mdec->phase == 1) {
        // マウスの1バイトを持っている状態
        if ((dat & 0xc8) == 0x08) {
            // 正しい1バイト目か調べる
            mdec->buf[0] = dat;
            mdec->phase = 2;
        }
        return 0;
    }
    if (mdec->phase == 2) {
        // マウスの2バイト目を持っている段階
        mdec->buf[1] = dat;
        mdec->phase = 3;
        return 0;
    }
    if (mdec->phase == 3) {
        // マウスの3バイト目を持っている段階
        mdec->buf[2] = dat;
        mdec->phase = 1;
        mdec->phase = 1;
        mdec->btn = mdec->buf[0] & 0x07;    // 00000111 にボタンが記録される
        mdec->x = mdec->buf[1];
        mdec->y = mdec->buf[2];
        if ((mdec->buf[0] & 0x10) != 0) {
            mdec->x |= 0xffffff00;
        }
        if ((mdec->buf[0] & 0x20) != 0) {
            mdec->y |= 0xffffff00;
        }
        mdec->y = - mdec->y;
        return 1;
    }
    return -1;
}

// PS/2マウスからの割り込み
void inthandler2c(int *esp) {
    unsigned char data;

    io_out8(PIC1_OCW2, 0x64);   // IRQ-12受付完了をPIC1に通知(スレーブ4番目)
    io_out8(PIC0_OCW2, 0x62);   // IRQ-02受付完了をPIC0に通知(マスタ2番)
    data = io_in8(PORT_KEYDAT);
    fifo8_put(&mousefifo, data);
    return;
}