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

void console_task(struct SHEET *sheet) {
    struct FIFO32 fifo;
    struct TIMER *timer;
    struct TASK *task = task_now();   // 自分自身をつかっていないときにスリープさせる -> アドレス欲しい
    int i, fifobuf[128], cursor_x = 8, cursor_c = COL8_000000;

    fifo32_init(&fifo, 128, fifobuf, task);
    timer = timer_alloc();
    timer_init(timer, &fifo, 1);
    timer_settime(timer, 50);

    for (;;) {
        io_cli();
        if (fifo32_status(&fifo) == 0) {
            task_sleep(task);
            io_sti();
        } else {
            i = fifo32_get(&fifo);
            io_sti();
            if (i <= 1) {   // カーソル用タイマ
                if (i != 0) {
                    timer_init(timer, &fifo, 0);    // 次は0にする
                    cursor_c = COL8_FFFFFF;
                } else {
                    timer_init(timer, &fifo, 1);    // 次は1にする
                    cursor_c = COL8_000000;
                }
                timer_settime(timer, 50);
                boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                sheet_refresh(sheet, cursor_x, 28, cursor_x + 8, 44);
            }
        }
    }
}

void HariMain(void) {
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    struct MOUSE_DEC mdec;
    struct FIFO32 fifo;
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct SHTCTL *shtctl;
    char s[40];
    int fifobuf[128];
    int mx = 120, my =120, i, j, count;
    int cursor_x, cursor_c;
    unsigned int memtotal;

    unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons;
    struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_cons;
    struct TASK *task_a, *task_cons;
    struct TIMER *timer;

    fifo32_init(&fifo, 128, fifobuf, 0);    // task_aのアドレスがまだわかんないから0にしておく，
    init_gdtidt();
    init_pic();
    // 1秒につき100回IRQ0が割り込みを行う
    init_pit();

    init_keyboard(&fifo, 256);
    enable_mouse(&fifo, 512, &mdec);

    io_sti();   // IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除

    io_out8(PIC0_IMR, 0xf8); /* PIT, PIC1とキーボード許可(11111000) */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可(11101111) */

    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);    // 0x00001000 - 0x0009efff
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    init_palette();
    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
    task_a = task_init(memman);
    fifo.task = task_a;
    task_run(task_a, 1, 0);

    // sht_back
    sht_back  = sheet_alloc(shtctl);
    buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    sheet_setbuf(sht_back,  buf_back,  binfo->scrnx, binfo->scrny, -1);   // 透明色なし
    init_screen(buf_back, binfo->scrnx, binfo->scrny);

    // sht_win_b
    // for (i = 0; i < 3; i++) {
    //     sht_win_b[i] = sheet_alloc(shtctl);
    //     buf_win_b = (unsigned char *) memman_alloc_4k(memman, 144 * 52);
    //     sheet_setbuf(sht_win_b[i], buf_win_b, 144, 52, -1);
    //     sprintf(s, "task_b%d", i);
    //     make_window8(buf_win_b, 144, 52, s, 0);
    //     task_b[i] = task_alloc();
    //     task_b[i]->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
    //     task_b[i]->tss.eip = (int) &task_b_main;
    //     task_b[i]->tss.es  = 1 * 8;
    //     task_b[i]->tss.cs  = 2 * 8;
    //     task_b[i]->tss.ss  = 1 * 8;
    //     task_b[i]->tss.ds  = 1 * 8;
    //     task_b[i]->tss.fs  = 1 * 8;
    //     task_b[i]->tss.gs  = 1 * 8;
    //     *((int *) (task_b[i]->tss.esp + 4)) = (int) sht_win_b[i];
    //     // task_run(task_b[i], 2, i + 1);   // hltできているかみるために止める
    // }

    // sht_cons
    sht_cons = sheet_alloc(shtctl);
    buf_cons = (unsigned char *) memman_alloc_4k(memman, 256 * 165);
    sheet_setbuf(sht_cons, buf_cons, 256, 165, -1);
    make_window8(buf_cons, 256, 165, "console", 0);
    make_textbox8(sht_cons, 8, 28, 240, 128, COL8_000000);
    task_cons = task_alloc();
    task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
    task_cons->tss.eip = (int) &console_task;
    task_cons->tss.es  = 1 * 8;
    task_cons->tss.cs  = 2 * 8;
    task_cons->tss.ss  = 1 * 8;
    task_cons->tss.ds  = 1 * 8;
    task_cons->tss.fs  = 1 * 8;
    task_cons->tss.gs  = 1 * 8;
    *((int *) (task_cons->tss.esp + 4)) = (int) sht_cons;
    task_run(task_cons, 2, 2);   // level=2, priority=2


    // sht_win
    sht_win   = sheet_alloc(shtctl);
    buf_win   = (unsigned char *) memman_alloc_4k(memman, 160 * 52);
    sheet_setbuf(sht_win, buf_win, 144, 52, -1);    // 透明色はなし
    make_window8(buf_win, 144, 52, "task_a", 1);   // ウィンドウを描く
    make_textbox8(sht_win, 8, 28, 128, 16, COL8_FFFFFF);
    cursor_x = 8;
    cursor_c = COL8_FFFFFF;
    timer = timer_alloc();
    timer_init(timer, &fifo, 1);
    timer_settime(timer, 50);

    // sht_mouse
    sht_mouse = sheet_alloc(shtctl);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); // 透明色番号99
    init_mouse_cursor8(buf_mouse, 99); // 背景色は99
    mx = (binfo->scrnx - 16) / 2; // 画面中央を求める
    my = (binfo->scrny - 28 - 16) / 2;

    sheet_slide(sht_back,   0,   0);
    sheet_slide(sht_cons,  32,   4);
    sheet_slide(sht_win,    8,  56);
    sheet_slide(sht_mouse, mx,  my);
    sheet_updown(sht_back,  0);
    sheet_updown(sht_cons,  1);
    sheet_updown(sht_win,   2);
    sheet_updown(sht_mouse, 3);
    sprintf(s, "(%d, %d)", mx, my);
    putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
    sprintf(s, "memory %d MB    free : %d KB",
        memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putfonts8_asc_sht(sht_back, 0, 32, COL8_00FFFF, COL8_008484, s, 40);

    for ( ; ; ) {
        io_cli();
        // FIFOが空っぽの時
        if (fifo32_status(&fifo) == 0) {
            task_sleep(task_a); // スリープ処理
            io_sti();   // 処理が終わってから割り込み禁止
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
            } else if (i <= 1) {    // カーソル用タイマ
                if (i != 0) {
                    timer_init(timer, &fifo, 0); // 次は0
                    cursor_c = COL8_000000;
                } else {
                    timer_init(timer, &fifo, 1);
                    cursor_c = COL8_FFFFFF;
                }
                timer_settime(timer, 50);
                boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
            }
        }
    }
}