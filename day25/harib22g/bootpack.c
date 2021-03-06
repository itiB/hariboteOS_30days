#include "bootpack.h"

#define CONS_MAX (2) // 25日目 harib22e

// ウィンドウタイトルの色やtask_aウィンドウのカーソルを制御
int keywin_off(struct SHEET *key_win, struct SHEET *sht_win, int cur_c, int cur_x) {
    change_wtitle8(key_win, 0);
    if (key_win == sht_win) {
        cur_c = -1; // カーソルを消す
        boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cur_x, 28, cur_x + 7, 43);
    } else {
        if ((key_win->flags & 0x20) != 0) {
            fifo32_put(&key_win->task->fifo, 3);    // コンソールのカーソルOFF
        }
    }
    return cur_c;
}

// ウィンドウタイトルの色やtask_aウィンドウのカーソルを制御
int keywin_on(struct SHEET *key_win, struct SHEET *sht_win, int cur_c) {
    change_wtitle8(key_win, 1);
    if (key_win == sht_win) {
        cur_c = COL8_000000;    // カーソルを消す
    } else {
        if ((key_win->flags & 0x20) != 0) {
            fifo32_put(&key_win->task->fifo, 2);    // コンソールのカーソルON
        }
    }
    return cur_c;
}

void HariMain(void) {
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    struct MOUSE_DEC mdec;
    struct FIFO32 fifo, keycmd; // キーボードに送るデータを管理する
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct SHTCTL *shtctl;
    struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_cons[CONS_MAX];

    unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons[CONS_MAX];
    struct TASK *task_a, *task_cons[CONS_MAX];
    struct TIMER *timer;
    struct CONSOLE *cons;
    struct SHEET *sht = 0, *key_win;
    char s[40];
    int fifobuf[128], keycmd_buf[32];
    int mx = 120, my =120, i, j, count, x, y;
    int mmx = -1, mmy = -1;
    int cursor_x, cursor_c;
    unsigned int memtotal;
    int key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;

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
    task_run(task_a, 1, 2);

    // shtctlの情報をapiでも使うために記録
    *((int *) 0x0fe4) = (int) shtctl;

    // sht_back
    sht_back  = sheet_alloc(shtctl);
    buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); // 透明色なし
    init_screen(buf_back, binfo->scrnx, binfo->scrny);

    // sht_cons
    for (i = 0; i < CONS_MAX; i++) {
        sht_cons[i] = sheet_alloc(shtctl);
        buf_cons[i] = (unsigned char *) memman_alloc_4k(memman, 256 * 165);
        sheet_setbuf(sht_cons[i], buf_cons[i], 256, 165, -1);
        make_window8(buf_cons[i], 256, 165, "console", 0);
        make_textbox8(sht_cons[i], 8, 28, 240, 128, COL8_000000);
        task_cons[i] = task_alloc();
        task_cons[i]->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
        task_cons[i]->tss.eip = (int) &console_task;
        task_cons[i]->tss.es  = 1 * 8;
        task_cons[i]->tss.cs  = 2 * 8;
        task_cons[i]->tss.ss  = 1 * 8;
        task_cons[i]->tss.ds  = 1 * 8;
        task_cons[i]->tss.fs  = 1 * 8;
        task_cons[i]->tss.gs  = 1 * 8;
        *((int *) (task_cons[i]->tss.esp + 4)) = (int) sht_cons[i];
        *((int *) (task_cons[i]->tss.esp + 8)) = (int) memtotal;
        task_run(task_cons[i], 2, 2);   // level=2, priority=2
        sht_cons[i]->task = task_cons[i]; // 送信先のFIFOを決める
        sht_cons[i]->flags |= 0x20; // カーソルありなしをFlagで決める(あり)
    }

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

    key_win = sht_win;

    // sht_mouse
    sht_mouse = sheet_alloc(shtctl);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); // 透明色番号99
    init_mouse_cursor8(buf_mouse, 99); // 背景色は99
    mx = (binfo->scrnx - 16) / 2; // 画面中央を求める
    my = (binfo->scrny - 28 - 16) / 2;

    sheet_slide(sht_back,     0,  0);
    sheet_slide(sht_cons[1], 56,  6);
    sheet_slide(sht_cons[0],  8,  2);
    sheet_slide(sht_win,     64, 56);
    sheet_slide(sht_mouse,   mx, my);
    sheet_updown(sht_back,    0);
    sheet_updown(sht_cons[1], 1);
    sheet_updown(sht_cons[0], 2);
    sheet_updown(sht_win,     3);
    sheet_updown(sht_mouse,   4);

    // 食い違いがないようにはじめにキーボードを設定
    fifo32_put(&keycmd, KEYCMD_LED);
    fifo32_put(&keycmd, key_leds);

    for (;;) {
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
            if (key_win->flags == 0) {
                // 入力ウィンドウが閉じられた
                key_win = shtctl->sheets[shtctl->top - 1];
                cursor_c = keywin_on(key_win, sht_win, cursor_c);
            }
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
                    if (key_win == sht_win) {  // タスクAのとき
                        if (cursor_x < 128) {
                            // 一文字表示してからカーソルを一つ進める
                            s[1] = 0;
                            putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
                            cursor_x += 8;
                        }
                    } else {    // タスクB(コンソールへ)
                        fifo32_put(&key_win->task->fifo, s[0] + 256);
                    }
                }
                if (i == 256 + 0x0e) {  // バックスペース
                    if (key_win == sht_win) {
                        // カーソルをスペースで消してから一つカーソルを戻す
                        putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
                        cursor_x -= 8;
                    } else {
                        // コンソールへ
                        fifo32_put(&key_win->task->fifo, 8 + 256);
                    }
                }
                if (i == 256 + 0x1c) {  // Enter
                    if (key_win != sht_win) {  // コンソールへ
                        fifo32_put(&key_win->task->fifo, 10 + 256);
                    }
                }
                if (i == 256 + 0x0f) {  // Tab
                    cursor_c = keywin_off(key_win, sht_win, cursor_c, cursor_x);
                    j = key_win->height - 1;
                    if (j == 0) {
                        j = shtctl->top - 1;
                    }
                    key_win = shtctl->sheets[j];
                    cursor_c = keywin_on(key_win, sht_win, cursor_c);
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
                if (i == 256 + 0x3b && key_shift != 0 && task_cons[0]->tss.ss0 != 0) {
                    // Ctrl c(Shift + F1)
                    cons = (struct CONSOLE *) *((int *) 0x0fec);
                    cons_putstr0(cons, "\nBreak(key) :\n");
                    io_cli();   // レジスタ変更
                    task_cons[0]->tss.eax = (int) &(task_cons[0]->tss.esp0);
                    task_cons[0]->tss.eip = (int) asm_end_app;
                    io_sti();
                }
                if (i == 256 + 0x57 && shtctl->top > 2) {
                    // F11 & 複数枚の下敷きがある
                    sheet_updown(shtctl->sheets[1], shtctl->top - 1);
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
                        // 左ボタンを押していたら
                        if (mmx < 0) {
                            // 通常モードのとき
                            // 上の下敷きから順番にマウスのさしている下敷きを探す
                            for (j = shtctl->top - 1; j > 0; j--) {
                                sht = shtctl->sheets[j];
                                x = mx - sht->vx0;
                                y = my - sht->vy0;
                                if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {
                                    if (sht->buf[y * sht->bxsize + x] != sht->col_inv) {
                                        sheet_updown(sht, shtctl->top -1);
                                        if (sht != key_win) {
                                            cursor_c = keywin_off(key_win, sht_win, cursor_c, cursor_x);
                                            key_win = sht;
                                            cursor_c = keywin_on(key_win, sht_win, cursor_c);
                                        }
                                        if (3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21) {
                                            mmx = mx;   // ウィンドウ移動モードへ
                                            mmy = my;
                                        }
                                        if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) {
                                            // [x]ボタンがクリックされた
                                            if ((sht->flags & 0x10) != 0) {
                                                // アプリの作ったウィンドウならば
                                                cons = (struct CONSOLE *) *((int *) 0x0fec);
                                                cons_putstr0(cons, "\nBreak (mouse):\n");
                                                io_cli();
                                                // 割り込みで消す
                                                task_cons[0]->tss.eax = (int) &(task_cons[0]->tss.esp0);
                                                task_cons[0]->tss.eip = (int) asm_end_app;
                                                io_sti();
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                        } else {
                            // ウィンドウ移動モード
                            x = mx - mmx;   // マウスの移動量を計算
                            y = my - mmy;
                            sheet_slide(sht, sht->vx0 + x, sht->vy0 + y);
                            mmx = mx;   // 移動後の座標に更新
                            mmy = my;
                        }
                    } else {
                        // 左ボタンを押していない
                        // 通常モード
                        mmx = -1;
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