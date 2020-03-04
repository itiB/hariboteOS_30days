#include "bootpack.h"
// #include <stdarg.h>

void HariMain(void) {
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    struct MOUSE_DEC mdec;
    struct FIFO32 fifo, keycmd; // キーボードに送るデータを管理する
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct SHTCTL *shtctl;
    struct CONSOLE *cons;
    char s[40];
    int fifobuf[128], keycmd_buf[32];
    int mx = 120, my =120, i, j, count;
    int cursor_x, cursor_c;
    unsigned int memtotal;
    int key_to = 0, key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;

    static char keytable0[] = {
        0,   '`', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,   0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0,   0,   'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', 0x27, 0,   0,  '\\', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
    };
    static char keytable1[] = {
        0,   '~', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,   0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,   0,   'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', 0x22, 0,   0,   '|', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
    };

    unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons;
    struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_cons;
    struct TASK *task_a, *task_cons;
    struct TIMER *timer;

    fifo32_init(&fifo, 128, fifobuf, 0);    // task_aのアドレスがまだわかんないから0にしておく，
    fifo32_init(&keycmd, 32, keycmd_buf, 0);
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
    *((int *) 0x0fe4) = (int) shtctl;

    // sht_back
    sht_back  = sheet_alloc(shtctl);
    buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    sheet_setbuf(sht_back,  buf_back,  binfo->scrnx, binfo->scrny, -1);   // 透明色なし
    init_screen(buf_back, binfo->scrnx, binfo->scrny);

    // sht_cons
    sht_cons = sheet_alloc(shtctl);
    buf_cons = (unsigned char *) memman_alloc_4k(memman, 256 * 165);
    sheet_setbuf(sht_cons, buf_cons, 256, 165, -1);
    make_window8(buf_cons, 256, 165, "console", 0);
    make_textbox8(sht_cons, 8, 28, 240, 128, COL8_000000);
    task_cons = task_alloc();
    task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
    task_cons->tss.eip = (int) &console_task;
    task_cons->tss.es  = 1 * 8;
    task_cons->tss.cs  = 2 * 8;
    task_cons->tss.ss  = 1 * 8;
    task_cons->tss.ds  = 1 * 8;
    task_cons->tss.fs  = 1 * 8;
    task_cons->tss.gs  = 1 * 8;
    *((int *) (task_cons->tss.esp + 4)) = (int) sht_cons;
    *((int *) (task_cons->tss.esp + 8)) = (int) memtotal;
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

    // 食い違いがないようにはじめにキーボードを設定
    fifo32_put(&keycmd, KEYCMD_LED);
    fifo32_put(&keycmd, key_leds);

    for ( ; ; ) {
        if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
            // キーボードコントローラーに送るデータがあるならば送る
            keycmd_wait = fifo32_get(&keycmd);
            wait_KBC_sendready();
            io_out8(PORT_KEYDAT, keycmd_wait);
        }
        io_cli();
        // FIFOが空っぽの時
        if (fifo32_status(&fifo) == 0) {
            task_sleep(task_a); // スリープ処理
            io_sti();   // 処理が終わってから割り込み禁止
        } else {
            i = fifo32_get(&fifo);
            io_sti();
            if (256 <= i && i <= 511) {   // キーボードデータ
                // sprintf(s, "%d", i - 256);
                // putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
                if (i < 256 + 0x80) {   // キーコードを文字コードに変換する
                    if (key_shift == 0) {
                        s[0] = keytable0[i - 256];
                    } else {
                        s[0] = keytable1[i - 256];
                    }
                } else {
                    s[0] = 0;
                }
                if ('A' <= s[0] && s[0] <= 'Z') {   // 入力がアルファベットのとき
                    if (((key_leds & 4) == 0 && key_shift == 0) ||
                        ((key_leds & 4) != 0 && key_shift != 0)) {
                            s[0] += 0x20;   // 大文字を小文字に変換
                    }
                }
                if (s[0] != 0) {    // 通常文字の時
                    if (key_to == 0) {  // タスクAのとき
                        if (cursor_x < 128) {
                            // 一文字表示してからカーソルを一つ進める
                            s[1] = 0;
                            putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
                            cursor_x += 8;
                        }
                    } else {    // タスクB
                        fifo32_put(&task_cons->fifo, s[0] + 256);
                    }
                }
                if (i == 256 + 0x0e) {  // バックスペース
                    if (key_to == 0 && cursor_x > 8) {
                        // カーソルをスペースで消してから一つカーソルを戻す
                        putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
                        cursor_x -= 8;
                    } else {
                        // コンソールへ
                        fifo32_put(&task_cons->fifo, 8 + 256);
                    }
                }
                if (i == 256 + 0x1c) {  // Enter
                    if (key_to != 0) {  // コンソールへ
                        fifo32_put(&task_cons->fifo, 10 + 256);
                    }
                }
                if (i == 256 + 0x0f) {  // Tab
                    if (key_to == 0) {
                        key_to = 1;
                        make_wtitle8(buf_win, sht_win->bxsize, "task_a", 0);
                        make_wtitle8(buf_cons, sht_cons->bxsize, "console", 1);
                        cursor_c = -1;  // カーソルを消す
                        boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cursor_x, 28, cursor_x + 7, 43);
                        fifo32_put(&task_cons->fifo, 2);    // コンソールのカーソルをONにする
                    } else {
                        key_to = 0;
                        make_wtitle8(buf_win, sht_win->bxsize, "task_a", 1);
                        make_wtitle8(buf_cons, sht_cons->bxsize, "console", 0);
                        cursor_c = COL8_000000;  // カーソルを表示
                        fifo32_put(&task_cons->fifo, 3);    // コンソールのカーソルをOFFにする
                    }
                    sheet_refresh(sht_win, 0 , 0, sht_win->bxsize, 21);
                    sheet_refresh(sht_cons, 0 , 0, sht_cons->bxsize, 21);
                }
                if (i == 256 + 0x2a) {  // 左シフトON
                    key_shift |= 1;
                }
                if (i == 256 + 0x36) {  // 右シフトON
                    key_shift |= 2;
                }
                if (i == 256 + 0xaa) {  // 左シフトOFF
                    key_shift &= ~1;
                }
                if (i == 256 + 0xb6) {  // 右シフトOFF
                    key_shift &= ~2;
                }
                if (i == 256 + 0x3a) {  // CapsLock
                    key_leds ^= 4;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x45) {  // NumLock
                    key_leds ^= 2;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x46) {  // ScrollLock
                    key_leds ^= 1;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0xfa) {  // キーボードがデータを無事に受け取った
                    keycmd_wait = -1;
                }
                if (i == 256 + 0xfe) {  // キーボードがデータを受け取れなかった
                    wait_KBC_sendready();
                    io_out8(PORT_KEYDAT, keycmd_wait);
                }
                if (i == 256 + 0x3b && key_shift != 0 && task_cons->tss.ss0 != 0) {
                    // Ctrl c(Shift + F1)
                    cons = (struct CONSOLE *) *((int *) 0x0fec);
                    cons_putstr0(cons, "\nBreak(key) :\n");
                    io_cli();   // レジスタ変更
                    task_cons->tss.eax = (int) &(task_cons->tss.esp0);
                    task_cons->tss.eip = (int) asm_end_app;
                    io_sti();
                }
                // カーソルの再表示
                if (cursor_c >= 0) {
                    boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                }
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
            } else if (512 <= i && i <= 767) {    // マウスデータ
                if (mouse_decode(&mdec, i - 512) != 0) {
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
                    // sprintf(s, "(%d, %d)", mx, my);
                    // putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
                    sheet_slide(sht_mouse, mx, my);
                    if ((mdec.btn & 0x01) != 0) {
                        // 左ボタンを押していたらsht_winを動かす
                        sheet_slide(sht_win, mx - 80, my - 8);
                    }
                }
            } else if (i <= 1) {    // カーソル用タイマ
                if (i != 0) {
                    timer_init(timer, &fifo, 0); // 次は0
                    if (cursor_c >= 0) {
                        cursor_c = COL8_000000;
                    }
                } else {
                    timer_init(timer, &fifo, 1);
                    if (cursor_c >= 0) {
                        cursor_c = COL8_FFFFFF;
                    }
                }
                timer_settime(timer, 50);
                if (cursor_c >= 0) {
                    boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                    sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
                }
            }
        }
    }
}