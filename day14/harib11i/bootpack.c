#include "bootpack.h"

// 背景色を塗りつぶして文字を描いてリフレッシュする
// x, y: 座標
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int color, int back_color, char *str, int len) {
    boxfill8(sht->buf, sht->bxsize, back_color, x, y, x + len * 8 - 1, y + 15);
    putfont8_asc(sht->buf, sht->bxsize, x, y, color, str);
    sheet_refresh(sht, x, y, x + len * 8, y + 16);
    return;
}

void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c) {
    int x1 = x0 + sx, y1 = y0 + sy;
    boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
    boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
    boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, c,           x0 - 1, y0 - 1, x1 + 0, y1 + 0);
    return;
}

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    struct MOUSE_DEC mdec;
    struct FIFO32 fifo;
    char s[40];
    int fifobuf[128];
    struct TIMER *timer, *timer2, *timer3;
    int mx = 120, my =120, i, j, count;
    int cursor_x, cursor_c;
    unsigned int memtotal;
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct SHTCTL *shtctl;
    struct SHEET *sht_back, *sht_mouse, *sht_win;
    unsigned char *buf_back, buf_mouse[256], *buf_win;

    init_gdtidt();
    init_pic();
    io_sti();   // IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除
    // fifo32_init(&keybuf, 32, keybuf);
    // fifo8_init(&mousefifo, 128, mousebuf);

    fifo32_init(&fifo, 128, fifobuf);
    init_keyboard(&fifo, 256);
    enable_mouse(&fifo, 512, &mdec);
    // 1秒につき100回IRQ0が割り込みを行う
    init_pit();
    io_out8(PIC0_IMR, 0xf8); /* PIT, PIC1とキーボード許可(11111000) */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可(11101111) */

    timer = timer_alloc();
    timer_init(timer, &fifo, 10);
    timer_settime(timer, 1000);
    timer2 = timer_alloc();
    timer_init(timer2, &fifo, 3);
    timer_settime(timer2, 300);
    timer3 = timer_alloc();
    timer_init(timer3, &fifo, 1);
    timer_settime(timer3, 50);

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
    buf_win   = (unsigned char *) memman_alloc_4k(memman, 160 * 52);    // ウィンドウ用
    sheet_setbuf(sht_back,  buf_back,  binfo->scrnx, binfo->scrny, -1);   // 透明色なし
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); // 透明色番号99
    sheet_setbuf(sht_win,   buf_win,   160, 52, -1);    // ウィンドウ用，透明色はなし

    init_screen(buf_back, binfo->scrnx, binfo->scrny);
    init_mouse_cursor8(buf_mouse, 99); // 背景色は99
    make_window8(buf_win, 160, 68, "window");   // ウィンドウを描く
    make_textbox8(sht_win, 8, 28, 144, 16, COL8_FFFFFF);
    cursor_x = 8;
    cursor_c = COL8_FFFFFF;
    sheet_slide(sht_back, 0, 0);
    mx = (binfo->scrnx - 16) / 2; // 画面中央を求める
    my = (binfo->scrny - 28 - 16) / 2;
    sheet_slide(sht_mouse, mx, my);
    sheet_slide(sht_win, 80, 72);   // ウィンドウが動くように
    sheet_updown(sht_back,  0);
    sheet_updown(sht_win,   1);
    sheet_updown(sht_mouse, 2);
    sprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
    sprintf(s, "memory %d MB    free : %d KB",
        memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putfonts8_asc_sht(sht_back, 0, 32, COL8_00FFFF, COL8_008484, s, 40);

    static char keytable[0x54] = {
        0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.'
    };



    for ( ; ; ) {
        io_cli();
        if (fifo32_status(&fifo) == 0) {
            io_stihlt();
        } else {
            i = fifo32_get(&fifo);
            io_sti();
            if (256 <= i && i <= 511) {   // キーボードデータ
                sprintf(s, "%d", i - 256);
                putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
                if (i < 256 + 0x54) {
                    if (keytable[i - 256] != 0 && cursor_x < 144) { // 通常文字ならば
                        // 一文字表示してからカーソルを一つ進める
                        s[0] = keytable[i - 256];
                        s[1] = 0;
                        putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
                        cursor_x += 8;
                    }
                }
                if (i == 256 + 0x0e && cursor_x > 8) {  // バックスペース
                    // カーソルをスペースで消してから一つカーソルを戻す
                    putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
                    cursor_x -= 8;
                }
                // カーソルの再表示
                boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
            } else if (512 <= i && i <= 767) {    // マウスデータ
                if (mouse_decode(&mdec, i - 512) != 0) {
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
                    putfonts8_asc_sht(sht_back, 32, 16, COL8_FFFFFF, COL8_008484, s, 15);

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
                    putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
                    sheet_slide(sht_mouse, mx, my);
                    if ((mdec.btn & 0x01) != 0) {
                        // 左ボタンを押していたらsht_winを動かす
                        sheet_slide(sht_win, mx - 80, my - 8);
                    }
                }
            } else if (i == 10) {
                putfonts8_asc_sht(sht_back, 0, 64, COL8_FFFFFF, COL8_008484, "10[sec]", 7);
                // sprintf(s, "%d", count);
                // putfonts8_asc_sht(sht_win, 40, 28, COL8_000000, COL8_C6C6C6, s, 10);
            } else if (i == 3) {
                putfonts8_asc_sht(sht_back, 0, 80, COL8_FFFFFF, COL8_008484, "3[sec]", 6);
                // count = 0; // 測定開始
            } else if (i <= 1) {    // カーソル用タイマ
                if (i != 0) {
                    timer_init(timer3, &fifo, 0); // 次は0
                    cursor_c = COL8_000000;
                } else {
                    timer_init(timer3, &fifo, 1);
                    cursor_c = COL8_FFFFFF;
                }
                timer_settime(timer3, 50);
                boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
            }
        }
    }
}