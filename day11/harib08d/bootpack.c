#include "bootpack.h"

// 画面にウィンドウを表示させる
void make_window8(unsigned char *buf, int xsize, int ysize, char *title) {
    static char closebtn[14][16] =  {
        "ooooooooooooooo@",
        "oQQQQQQQQQQQQQ$@",
        "oQQQQQQQQQQQQQ$@",
        "oQQQ@@QQQQ@@QQ$@",
        "oQQQQ@@OO@@QQQ$@",
        "oQQQQQ@@@@QQQQ$@",
        "oQQQQQQ@@QQQQQ$@",
        "oQQQQQ@@@@QQQQ$@",
        "oQQQQ@@OO@@QQQ$@",
        "oQQQ@@QQQQ@@QQ$@",
        "oQQQQQQQQQQQQQ$@",
        "oQQQQQQQQQQQQQ$@",
        "o$$$$$$$$$$$$$$@",
        "@@@@@@@@@@@@@@@@"
    };
    int x, y;
    char c;

    boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         xsize - 1, 0        );
    boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         xsize - 2, 1        );
    boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         0,         ysize - 1);
    boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         1,         ysize - 2);
    boxfill8(buf, xsize, COL8_848484, xsize - 2, 1,         xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, xsize - 1, 0,         xsize - 1, ysize - 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 2,         2,         xsize - 3, ysize - 3);
    boxfill8(buf, xsize, COL8_000084, 3,         3,         xsize - 4, 20       );
    boxfill8(buf, xsize, COL8_848484, 1,         ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, 0,         ysize - 1, xsize - 1, ysize - 1);
    putfont8_asc(buf, xsize, 24, 4, COL8_FFFFFF, title);
    for (y = 0; y < 14; y++) {
        for (x = 0; x < 16; x++) {
            c = closebtn[y][x];
            if      (c == '@') { c = COL8_000000; }
            else if (c == '$') { c = COL8_848484; }
            else if (c == 'Q') { c = COL8_C6C6C6; }
            else               { c = COL8_FFFFFF; }
            buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
        }
    }
    return;
}

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
    struct MOUSE_DEC mdec;
    char s[40];
    char mcursor[256];
    char keybuf[32];
    char mousebuf[128];
    int mx = 120, my =120, i, j;
    unsigned int memtotal;
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

    struct SHTCTL *shtctl;
    struct SHEET *sht_back, *sht_mouse, *sht_win;
    unsigned char *buf_back, buf_mouse[256], *buf_win;

    init_gdtidt();
    init_pic();
    io_sti();   // IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除
    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);
    io_out8(PIC0_IMR, 0xf9); /* pic1とキーボード許可 */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可 */

    init_keyboard();
    enable_mouse(&mdec);
    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);    // 0x00001000 - 0x0009efff
    memman_free(memman, 0x00400000, memtotal - 0x00400000);
    init_palette();

    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
    sht_back  = sheet_alloc(shtctl);
    sht_mouse = sheet_alloc(shtctl);
    sht_win   = sheet_alloc(shtctl);    // ウィンドウ用
    buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    buf_win   = (unsigned char *) memman_alloc_4k(memman, 160 * 68);    // ウィンドウ用
    sheet_setbuf(sht_back,  buf_back,  binfo->scrnx, binfo->scrny, -1);   // 透明色なし
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); // 透明色番号99
    sheet_setbuf(sht_win,   buf_win,   160, 68, -1);    // ウィンドウ用，透明色はなし

    init_screen(buf_back, binfo->scrnx, binfo->scrny);
    init_mouse_coursor8(buf_mouse, 99); // 背景色は99
    make_window8(buf_win, 160, 68, "window");   // ウィンドウを描く
    putfont8_asc(buf_win, 160, 24, 28, COL8_000000, "Welcome to");
    putfont8_asc(buf_win, 160, 24, 44, COL8_000000, "  Haribote-OS");
    sheet_slide(sht_back, 0, 0);
    mx = (binfo->scrnx - 16) / 2; // 画面中央を求める
    my = (binfo->scrny - 28 - 16) / 2;
    sheet_slide(sht_mouse, mx, my);
    sheet_slide(sht_win, 80, 72);   // ウィンドウが動くように
    sheet_updown(sht_back,  0);
    sheet_updown(sht_win,   1);
    sheet_updown(sht_mouse, 2);
    sprintf(s, "(%d, %d)", mx, my);
    putfont8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
    sprintf(s, "memory %d MB    free : %d KB",
        memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putfont8_asc(buf_back, binfo->scrnx, 0, 32, COL8_00FFFF, s);
    sheet_refresh(sht_back, 0, 0, binfo->scrnx, 48);

    for ( ; ; ) {
        io_cli();
        if ((fifo8_status(&keyfifo) + fifo8_status(&mousefifo)) == 0) {
            io_stihlt();
        } else {
            if (fifo8_status(&keyfifo) != 0) {
                i = fifo8_get(&keyfifo);
                io_sti();
                sprintf(s, "%x", i);
                boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
                putfont8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
                sheet_refresh(sht_back, 0, 16, 15, 32);
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
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfont8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
                    sheet_refresh(sht_back, 32, 16, 32 + 15 * 8, 32);

                    // マウスカーソルの移動
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo->scrnx - 1) {
                        mx = binfo->scrnx - 1;
                    }
                    if (my > binfo->scrny - 1) {
                        my = binfo->scrny - 1;
                    }
                    sprintf(s, "(%d, %d)", mx, my);
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15); // 座標を消す
                    putfont8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s); // 座標を書く
                    sheet_refresh(sht_back, 0, 0, 80, 16);
                    sheet_slide(sht_mouse, mx, my);
                }
            }
        }
    }
}