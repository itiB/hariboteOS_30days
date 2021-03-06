#include "bootpack.h"

// 背景色を塗りつぶして文字を描いてリフレッシュする
// x, y: 座標
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int color, int back_color, char *str, int len) {
    boxfill8(sht->buf, sht->bxsize, back_color, x, y, x + len * 8 - 1, y + 15);
    putfont8_asc(sht->buf, sht->bxsize, x, y, color, str);
    sheet_refresh(sht, x, y, x + len * 8, y + 16);
    return;
}

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    struct MOUSE_DEC mdec;
    struct FIFO8 timerfifo, timerfifo2, timerfifo3;
    char s[40];
    char mcursor[256];
    char keybuf[32];
    char mousebuf[128];
    char timerbuf[8], timerbuf2[8], timerbuf3[8];
    struct TIMER *timer, *timer2, *timer3;
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

    // 1秒につき100回IRQ0が割り込みを行う
    init_pit();
    io_out8(PIC0_IMR, 0xf8); /* PIT, PIC1とキーボード許可(11111000) */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可(11101111) */

    fifo8_init(&timerfifo, 8, timerbuf);
    timer = timer_alloc();
    timer_init(timer, &timerfifo, 1);
    timer_settime(timer, 1000);
    fifo8_init(&timerfifo2, 8, timerbuf2);
    timer2 = timer_alloc();
    timer_init(timer2, &timerfifo2, 1);
    timer_settime(timer2, 300);
    fifo8_init(&timerfifo3, 8, timerbuf3);
    timer3 = timer_alloc();
    timer_init(timer3, &timerfifo3, 1);
    timer_settime(timer3, 50);

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
    buf_win   = (unsigned char *) memman_alloc_4k(memman, 160 * 52);    // ウィンドウ用
    sheet_setbuf(sht_back,  buf_back,  binfo->scrnx, binfo->scrny, -1);   // 透明色なし
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); // 透明色番号99
    sheet_setbuf(sht_win,   buf_win,   160, 52, -1);    // ウィンドウ用，透明色はなし

    init_screen(buf_back, binfo->scrnx, binfo->scrny);
    init_mouse_cursor8(buf_mouse, 99); // 背景色は99
    make_window8(buf_win, 160, 68, "counter");   // ウィンドウを描く
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

    for ( ; ; ) {
        // count++;
        sprintf(s, "%d", timerctl.count);
        putfonts8_asc_sht(sht_win, 40, 28, COL8_000000, COL8_C6C6C6, s, 10);

        io_cli();
        if ((fifo8_status(&keyfifo) + fifo8_status(&mousefifo) 
            + fifo8_status(&timerfifo) + fifo8_status(&timerfifo2) + fifo8_status(&timerfifo3)) == 0) {
            io_sti();
        } else {
            if (fifo8_status(&keyfifo) != 0) {
                i = fifo8_get(&keyfifo);
                io_sti();
                sprintf(s, "%d", i);
                putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
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
                }
            } else if (fifo8_status(&timerfifo) != 0) {
                i = fifo8_get(&timerfifo);  // 取り合えず読み込む
                io_sti();
                putfont8_asc(buf_back, binfo->scrnx, 0, 64, COL8_FFFFFF, "10[sec]");
                sheet_refresh(sht_back, 0, 64, 56, 80);
            } else if (fifo8_status(&timerfifo2) != 0) {
                i = fifo8_get(&timerfifo2);  // 取り合えず読み込む
                io_sti();
                putfont8_asc(buf_back, binfo->scrnx, 0, 80, COL8_FFFFFF, "3[sec]");
                sheet_refresh(sht_back, 0, 80, 48, 96);
            } else if (fifo8_status(&timerfifo3) != 0) {    // カーソルもどき
                i = fifo8_get(&timerfifo3);  // 取り合えず読み込む
                io_sti();
                if (i != 0) {
                    timer_init(timer3, &timerfifo3, 0); // 次は0
                    boxfill8(buf_back, binfo->scrnx, COL8_FFFFFF, 8, 96, 15, 111);
                } else {
                    timer_init(timer3, &timerfifo3, 1); // 次は1
                    boxfill8(buf_back, binfo->scrnx, COL8_FFFFFF, 8, 96, 15, 111);
                }
                timer_settime(timer3, 50);
                sheet_refresh(sht_back, 8, 96, 16, 112);
            }
        }
    }
}