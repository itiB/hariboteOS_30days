#include "bootpack.h"

#define PORT_KEYDAT             0x0060
#define PORT_KEYSTA             0x0064
#define PORT_KEYCMD             0x0064  // コマンドを受け付けるようになるとデータ下から2bit目が0になる
#define KEYSTA_SEND_NOTREADY    0x02
#define KEYCMD_WRITE_MODE       0x60    // モード設定するよ命令
#define KBC_MODE                0x47    // マウス利用するためのモード
#define KEYCMD_SENDTO_MOUSE     0xd4    // この命令の次の命令をマウスに送る
#define MOUSECMD_ENABLE         0xf4

struct MOUSE_DEC {
    unsigned char buf[3], phase;
    int x, y, btn;
};

void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

void wait_KBC_sendready(void);
void init_keyboard(void);

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
    struct MOUSE_DEC mdec;
    char s[40];
    char mcursor[256];
    char keybuf[32];
    char mousebuf[128];
    int mx = 120, my =120, i, j;

    init_gdtidt();
    init_pic();
    io_sti();

    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);

    io_out8(PIC0_IMR, 0xf9); /* pic1とキーボード許可 */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可 */

    init_keyboard();

    init_palette();
    init_screen(binfo -> vram, binfo -> scrnx, binfo -> scrny);


    enable_mouse(&mdec);

    init_mouse_coursor8(mcursor, COL8_008484);


    for ( ; ; ) {
        io_cli();

        if ((fifo8_status(&keyfifo) + fifo8_status(&mousefifo)) == 0) {
            io_stihlt();
        } else {
            if (fifo8_status(&keyfifo) != 0) {
                i = fifo8_get(&keyfifo);
                io_sti();
                sprintf(s, "%x", i);
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
                putfont8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
            } else if (fifo8_status(&mousefifo) != 0) {
                i = fifo8_get(&mousefifo);
                io_sti();
                if (mouse_decode(&mdec, i) != 0) {
                    // マウスデータ3バイトがそろったので表示
                    sprintf(s, "[lcr %d %d]", mdec.x, mdec.y);
                    if ((mdec.btn & 0x01) != 0) {
                        s[1] = 'L';
                    }
                    if ((mdec.btn & 0x02) != 0) {
                        s[3] = 'R';
                    }
                    if ((mdec.btn & 0x04) != 0) {
                        s[2] = 'C';
                    }
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfont8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);

                    // マウスカーソルの移動
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);     // マウスを消す
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo -> scrnx - 16) {
                        mx = binfo -> scrnx - 16;
                    }
                    if (my > binfo -> scrny - 16) {
                        my = binfo -> scrny - 16;
                    }
                    sprintf(s, "(%d, %d)", mx, my);
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15); // 座標を消す
                    putfont8_asc(binfo->vram, binfo->scrnx,  0, 0, COL8_FFFFFF, s); // 座標を書く
                    putblock8_8(binfo -> vram, binfo -> scrnx, 16, 16, mx, my, mcursor, 16);    // マウスを描く
                }
            }
        }
    }
}

void enable_mouse(struct MOUSE_DEC *mdec) {
    // マウスの有効化
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);  // マウス使っていいよ～～～
    mdec -> phase = 0;  // マウスの返事(0xfa)を持っている状態
    return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat) {
    if (mdec -> phase == 0) {
        // マウスの0xfaを持っている
        if (dat == 0xfa) {
            mdec -> phase = 1;
        }
        return 0;
    }
    if (mdec -> phase == 1) {
        // マウスの1バイトを持っている状態
        if ((dat & 0xc8) == 0x08) {
            // 正しい1バイト目か調べる
            mdec -> buf[0] = dat;
            mdec -> phase = 2;
        }
        return 0;
    }
    if (mdec -> phase == 2) {
        // マウスの2バイト目を持っている段階
        mdec -> buf[1] = dat;
        mdec -> phase = 3;
        return 0;
    }
    if (mdec -> phase == 3) {
        // マウスの3バイト目を持っている段階
        mdec -> buf[2] = dat;
        mdec -> phase = 1;
        mdec -> phase = 1;
        mdec -> btn = mdec -> buf[0] & 0x07;    // 00000111 にボタンが記録される
        mdec -> x = mdec -> buf[1];
        mdec -> y = mdec -> buf[2];
        if ((mdec -> buf[0] & 0x10) != 0) {
            mdec -> x |= 0xffffff00;
        }
        if ((mdec -> buf[0] & 0x20) != 0) {
            mdec -> y |= 0xffffff00;
        }
        mdec -> y = - mdec -> y;
        return 1;
    }
    return -1;
}

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
