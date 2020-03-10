#include "bootpack.h"

struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct SHEET *sht = sheet_alloc(shtctl);
    unsigned char *buf = (unsigned char *) memman_alloc_4k(memman, 256 * 165);
    struct TASK *task = task_alloc();
    int *cons_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
    sheet_setbuf(sht, buf, 256, 165, -1); // 透明色なし
    make_window8(buf, 256, 165, "console", 0);
    make_textbox8(sht, 8, 28, 240, 128, COL8_000000);
    task->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
    task->tss.eip = (int) &console_task;
    task->tss.es  = 1 * 8;
    task->tss.cs  = 2 * 8;
    task->tss.ss  = 1 * 8;
    task->tss.ds  = 1 * 8;
    task->tss.fs  = 1 * 8;
    task->tss.gs  = 1 * 8;
    *((int *) (task->tss.esp + 4)) = (int) sht;
    *((int *) (task->tss.esp + 8)) = (int) memtotal;
    task_run(task, 2, 2);   // level=2, priority=2
    sht->task = task; // 送信先のFIFOを決める
    sht->flags |= 0x20; // カーソルありなしをFlagで決める(あり)
    fifo32_init(&task->fifo, 128, cons_fifo, task);
    return sht;
}

// ウィンドウタイトルの色やtask_aウィンドウのカーソルを制御
void keywin_off(struct SHEET *key_win) {
    change_wtitle8(key_win, 0);
    if ((key_win->flags & 0x20) != 0) {
        fifo32_put(&key_win->task->fifo, 3);    // コンソールのカーソルOFF
    }
    return;
}

// ウィンドウタイトルの色やtask_aウィンドウのカーソルを制御
void keywin_on(struct SHEET *key_win) {
    change_wtitle8(key_win, 1);
    if ((key_win->flags & 0x20) != 0) {
        fifo32_put(&key_win->task->fifo, 2);    // コンソールのカーソルON
    }
    return;
}

void HariMain(void) {
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    struct MOUSE_DEC mdec;
    struct FIFO32 fifo, keycmd; // キーボードに送るデータを管理する
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct SHTCTL *shtctl;
    struct SHEET *sht_back, *sht_mouse, *sht_win;

    unsigned char *buf_back, buf_mouse[256]; //, *buf_cons[CONS_MAX];
    struct TASK *task_a, *task; //, *task_cons[CONS_MAX]
    struct CONSOLE *cons;
    struct SHEET *sht = 0, *key_win;
    char s[40];
    int fifobuf[128], keycmd_buf[32]; //, *cons_fifo[CONS_MAX];
    int mx, my, i, j, count, x, y, new_mx = -1, new_my = 0, new_wx = 0x7fffffff, new_wy = 0;
    int mmx = -1, mmy = -1, mmx2 = 0;
    unsigned int memtotal;
    int key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;

    static char keytable0[] = {
        0,   '`', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x08, 0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0x0a,   0,   'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', 0x27, 0,   0,  '\\', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
    };
    static char keytable1[] = {
        0,   '~', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0x08,   0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0x0a,   0,   'A', 'S',
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
    key_win = open_console(shtctl, memtotal);

    // sht_mouse
    sht_mouse = sheet_alloc(shtctl);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); // 透明色番号99
    init_mouse_cursor8(buf_mouse, 99); // 背景色は99
    mx = (binfo->scrnx - 16) / 2; // 画面中央を求める
    my = (binfo->scrny - 28 - 16) / 2;

    sheet_slide(sht_back,   0,  0);
    sheet_slide(key_win,   32,  4);
    sheet_slide(sht_mouse, mx, my);
    sheet_updown(sht_back,  0);
    sheet_updown(key_win,   1);
    sheet_updown(sht_mouse, 2);
    keywin_on(key_win);

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
        if (fifo32_status(&fifo) == 0) {
            // FIFOが空っぽの時
            // 保留している描画を実行
            if (new_mx >= 0) {
                io_sti();
                sheet_slide(sht_mouse, new_mx, new_my);
                new_mx = -1;
            } else if (new_wx != 0x7fffffff) {
                io_sti();
                sheet_slide(sht, new_wx, new_wy);
                new_wx = 0x7fffffff;
            } else {
                task_sleep(task_a); // スリープ処理
                io_sti();   // 処理が終わってから割り込み禁止
            }
        } else {
            i = fifo32_get(&fifo);
            io_sti();
            if (key_win->flags == 0) {
                // 入力ウィンドウが閉じられた
                key_win = shtctl->sheets[shtctl->top - 1];
                // cursor_c = keywin_on(key_win, sht_win, cursor_c);
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
                if (s[0] != 0) {    // 通常文字, バックスペース, enter
                    fifo32_put(&key_win->task->fifo, s[0] + 256);
                }
                if (i == 256 + 0x0f) {  // Tab
                    keywin_off(key_win);
                    j = key_win->height - 1;
                    if (j == 0) {
                        j = shtctl->top - 1;
                    }
                    key_win = shtctl->sheets[j];
                    keywin_on(key_win);
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
                if (i == 256 + 0x3b && key_shift != 0) {
                    task = key_win->task;
                    if (task != 0 && task->tss.ss0 != 0) {
                        // Ctrl c(Shift + F1)
                        // cons = (struct CONSOLE *) *((int *) 0x0fec);
                        cons_putstr0(cons, "\nBreak(key) :\n");
                        io_cli();   // レジスタ変更
                        task->tss.eax = (int) &(task->tss.esp0);
                        task->tss.eip = (int) asm_end_app;
                        io_sti();
                    }
                }
                if (i == 256 + 0x3c && key_shift != 0) {
                    // shift + f2
                    // 新しく作ったコンソールを入力選択状態にする
                    keywin_off(key_win);
                    key_win = open_console(shtctl, memtotal);
                    sheet_slide(key_win, 32, 4);
                    sheet_updown(key_win, shtctl->top);
                    keywin_on(key_win);
                }
                if (i == 256 + 0x57 && shtctl->top > 2) {
                    // F11 & 複数枚の下敷きがある
                    sheet_updown(shtctl->sheets[1], shtctl->top - 1);
                }
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
                    sheet_slide(sht_mouse, mx, my);
                    new_mx = mx;
                    new_my = my;
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
                                            keywin_off(key_win);
                                            key_win = sht;
                                            keywin_on(key_win);
                                        }
                                        if (3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21) {
                                            mmx = mx; // ウィンドウ移動モードへ
                                            mmy = my;
                                            mmx2 = sht->vx0;
                                            new_wy = sht->vy0;
                                        }
                                        if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) {
                                            // [x]ボタンがクリックされた
                                            if ((sht->flags & 0x10) != 0) {
                                                // アプリの作ったウィンドウならば
                                                task = sht->task;
                                                // cons = (struct CONSOLE *) *((int *) 0x0fec);
                                                cons_putstr0(task->cons, "\nBreak (mouse):\n");
                                                io_cli();
                                                task->tss.eax = (int) &(task->tss.esp0);
                                                task->tss.eip = (int) asm_end_app;
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
                            new_wx = (mmx2 + x + 2) & ~3;
                            new_wy = new_wy + y;
                            mmy = my;
                        }
                    } else {
                        // 左ボタンを押していない
                        // 通常モード
                        mmx = -1;
                        if (new_wx != 0x7fffffff) {
                            sheet_slide(sht, new_wx, new_wy); // 一度確定
                            new_wx = 0x7fffffff;
                        }
                    }
                }
            }
        }
    }
}