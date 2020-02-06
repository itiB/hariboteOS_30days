#include "bootpack.h"

#define PORT_KEYDAT             0x0060
#define PORT_KEYSTA             0x0064
#define PORT_KEYCMD             0x0064  // コマンドを受け付けるようになるとデータ下から2bit目が0になる
#define KEYSTA_SEND_NOTREADY    0x02
#define KEYCMD_WRITE_MODE       0x60    // モード設定するよ命令
#define KBC_MODE                0x47    // マウス利用するためのモード

void wait_KBC_sendready(void) {
    // キーボードコントローラがデータ送信可能になるのを待つ(制御回路)
    // 命令CPUから送りすぎると回路が処理しきれないから受け付けるまで待つ
    for (;;) {
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
            break;
        }
    }
    return;
}

void init_keyboard(void) {
    // キーボードコントローラの初期化
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}

#define KEYCMD_SENDTO_MOUSE     0xd4    // この命令の次の命令をマウスに送る
#define MOUSECMD_ENABLE         0xf4

void enable_mouse(void) {
    // マウスの有効化
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);  // マウス使っていいよ～～～
    return;     // 返事りょか～～(ACK(0xfa))
}

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
    char s[40];
    char mcursor[256];
    char keybuf[32];
    int mx = 120, my =120, i, j;

    init_gdtidt();
    init_pic();
    io_sti();

    fifo8_init(&keyfifo, 32, keybuf);

    io_out8(PIC0_IMR, 0xf9); /* pic1とキーボード許可 */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可 */

    init_keyboard();

    init_palette();
    init_screen(binfo -> vram, binfo -> scrnx, binfo -> scrny);

    putfont8_asc(binfo -> vram, binfo -> scrnx,  9,  9, COL8_000000, "Haribote OS.");
    putfont8_asc(binfo -> vram, binfo -> scrnx,  8,  8, COL8_FFFFFF, "Haribote OS.");

    enable_mouse();

    sprintf(s, "scrnx = %d", binfo -> scrnx);
    putfont8_asc(binfo -> vram, binfo -> scrnx, 16, 64, COL8_FFFFFF, s);

    init_mouse_coursor8(mcursor, COL8_008484);
    putblock8_8(binfo -> vram, binfo -> scrnx, 16, 16, mx, my, mcursor, 16);

    for ( ; ; ) {
        io_cli();

        if (fifo8_status(&keyfifo) == 0) {
            io_stihlt();
        } else {
            i = fifo8_get(&keyfifo);
            io_sti();
            sprintf(s, "%x", i);
            boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
            putfont8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
        }
    }
}
