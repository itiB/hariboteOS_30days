#include "bootpack.h"

void console_task(struct SHEET *sheet, unsigned int memtotal) {
    struct TIMER *timer;
    struct TASK *task = task_now();   // 自分自身をつかっていないときにスリープさせる -> アドレス欲しい
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    int i, *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
    struct CONSOLE cons;
    struct FILEHANDLE fhandle[8];
    char cmdline[30];
    cons.sht = sheet;
    cons.cur_x =  8;
    cons.cur_y = 28;
    cons.cur_c = -1;
    // taskにconsole情報を記録する
    task->cons = &cons;
    task->cmdline = cmdline;

    if (cons.sht != 0) {
        cons.timer = timer_alloc();
        timer_init(cons.timer, &task->fifo, 1);
        timer_settime(cons.timer, 50);
    }
    file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));

    // fhandleの初期化
    for (i = 0; i < 8; i++) {
        fhandle[i].buf = 0; // 未使用マーク
    }
    task->fhandle = fhandle;
    task->fat = fat;

    // プロンプトの表示
    cons_putchar(&cons, '>', 1);

    for (;;) {
        io_cli();
        if (fifo32_status(&task->fifo) == 0) {
            task_sleep(task);
            io_sti();
        } else {
            i = fifo32_get(&task->fifo);
            io_sti();
            if (i <= 1 && cons.sht != 0) {   // カーソル用タイマ
                if (i != 0) {
                    timer_init(cons.timer, &task->fifo, 0);    // 次は0にする
                    if (cons.cur_c >= 0) {
                        cons.cur_c = COL8_FFFFFF;
                    }
                } else {
                    timer_init(cons.timer, &task->fifo, 1);    // 次は1にする
                    if (cons.cur_c >= 0) {
                        cons.cur_c = COL8_000000;
                    }
                }
                timer_settime(cons.timer, 50);
            }
            if (i == 2) {
                // カーソルON
                cons.cur_c = COL8_FFFFFF;
            }
            if (i == 3) {
                // カーソルOFF
                if (cons.sht != 0) {
                    boxfill8(cons.sht->buf, cons.sht->bxsize, COL8_000000, \
                        cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
                }
                cons.cur_c = -1;
            }
            if (i == 4) {
                cmd_exit(&cons, fat);
            }
            if (256 <= i && i <= 511) { // キーボードデータ
                if (i == 8 + 256) {
                    // バックスペースなら
                    if (cons.cur_x > 16) {
                        // カーソルをスペースでけしてカーソルを戻す
                        cons_putchar(&cons, ' ', 0);
                        cons.cur_x -= 8;
                    }
                } else if (i == 10 + 256) {
                    // Enter
                    // まずはカーソルを消す
                    cons_putchar(&cons, ' ', 0);
                    cmdline[cons.cur_x / 8 - 2] = 0;  // 入力された文字を初期化
                    cons_newline(&cons);
                    cons_runcmd(cmdline, &cons, fat, memtotal);
                    if (cons.sht == 0) {
                        cmd_exit(&cons, fat);
                    }
                    cons_putchar(&cons, '>', 1);
                } else {
                    // 一般文字
                    if (cons.cur_x < 240) {
                        // 一般文字を表示してからカーソルを進める
                        cmdline[cons.cur_x / 8 - 2] = i - 256;
                        cons_putchar(&cons, i - 256, 1);
                    }
                }
            }
            // カーソルの再表示
            if (cons.sht != 0) {
                if (cons.cur_c >= 0) {
                    boxfill8(cons.sht->buf, cons.sht->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
                }
                sheet_refresh(cons.sht, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
            }
        }
    }
}

// 入力された文字を1文字みて処理
void cons_putchar(struct CONSOLE *cons, int chr, char move) {
    char s[2];
    s[0] = chr;
    s[1] = 0;
    if (s[0] == 0x09) {
        // tab
        for (;;) {
            if (cons->sht != 0) {
                putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
            }
            cons->cur_x += 8;
            if(cons->cur_x == 8 + 240) cons_newline(cons);
            if (((cons->cur_x - 8) & 0x1f) == 0) {
                break;  // 32で割り切れたらBreak
            }
        }
    } else if (s[0] == 0x0a) {
        // 改行
        cons_newline(cons);
    } else if (s[0] == 0x0d) {
        // 復帰
    } else {
        if (cons->sht != 0) {
            putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
        }
        if (move != 0) {
            // moveが0のときはカーソルを進めない
            cons->cur_x += 8;
            if (cons->cur_x == 8 + 240) {
                cons_newline(cons);
            }
        }
    }
    return;
}

// 改行する，一番下まで言ったらスクロールする
void cons_newline(struct CONSOLE *cons) {
    int x, y;
    struct SHEET *sheet = cons->sht;

    if (cons->cur_y < 28 + 112) {
        // 次の行へ
        cons->cur_y += 16;
    } else {
        // スクロ――――ル！
        // ずらし作業
        if (cons->sht != 0) {
            for (y = 28; y < 28 + 112; y++) {
                for (x = 8; x < 8 + 240; x++) {
                    sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
                }
            }
            // 塗りつぶす
            for (y = 28 + 112; y < 28 + 128; y++) {
                for (x = 8; x < 8 + 240; x++) {
                    sheet->buf[x + y * sheet->bxsize] = COL8_000000;
                }
            }
            sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
        }
    }
    cons->cur_x = 8;
    return;
}

// コマンド実行
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal) {
    if (_strcmp(cmdline, "mem") == 0) {
        cmd_mem(cons, memtotal);
    } else if (_strcmp(cmdline, "cls") == 0) {
        cmd_cls(cons);
    } else if (_strcmp(cmdline, "ls") == 0) {
        cmd_ls(cons);
    } else if (_strcmp(cmdline, "exit") == 0) {
        cmd_exit(cons, fat);
    } else if (_strncmp(cmdline, "start ", 6) == 0) {
        cmd_start(cons, cmdline, memtotal);
    } else if (_strncmp(cmdline, "ncst ", 5) == 0) {
        cmd_ncst(cons, cmdline, memtotal);
    } else if (cmdline[0] != 0) {
        if (cmd_app(cons, fat, cmdline) == 0) {
            // コマンドではない & アプリでもない & 空行ではない
            cons_putstr0(cons, "Bad command.\n\n");
        }
    }
    return;
}

// memコマンド
void cmd_mem(struct CONSOLE *cons, unsigned int memtotal) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    char s[60];

    sprintf(s, "total %dMB\nfree  %dKB\n\n", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    cons_putstr0(cons, s);
    return;
}

void cmd_cls(struct CONSOLE *cons) {
    int x, y;
    struct SHEET *sheet = cons->sht;
    for (y = 28; y < 28 + 128; y++) {
       for (x = 8; x < 8 + 240; x++) {
           sheet->buf[x + y * sheet->bxsize] = COL8_000000;
       }
    }
    sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
    cons->cur_y = 28;
    return;
}

// lsコマンド
void cmd_ls(struct CONSOLE *cons) {
    struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
    int x, y;
    char s[30];
    for (x = 0; x < 224; x++) {
        if (finfo[x].name[0] == 0x00) {
            break;
        }
        if (finfo[x].name[0] != 0xe5) {
            if ((finfo[x].type & 0x18) == 0) {
                sprintf(s, "filename.ext   %d\n", finfo[x].size);
                for (y = 0; y < 8; y++) {
                    s[y] = finfo[x].name[y];
                }
                s[ 9] = finfo[x].ext[0];
                s[10] = finfo[x].ext[1];
                s[11] = finfo[x].ext[2];
                cons_putstr0(cons, s);
            }
        }
    }
    cons_newline(cons);
}

// exitコマンド
void cmd_exit(struct CONSOLE *cons, int *fat) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct TASK *task = task_now();
    struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
    struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0x0fec); // consを消すことをお願いするタスクAのFIFO
    timer_cancel(cons->timer); // カーソル点滅のためのタイマをキャンセル
    memman_free_4k(memman, (int) fat, 4 * 2880); // FATのメモリを解放
    io_cli();
    if (cons->sht != 0) {
        // コンソールがあるならば下敷きの場所を与える
        fifo32_put(fifo, cons->sht - shtctl->sheets0 + 768); // 768~1023のデータ
    } else {
        // コンソールが無ければタスク構造体自体がどこかを伝える
        fifo32_put(fifo, task - taskctl->tasks0 + 1024); // 1024~2023
    }
    io_sti();
    for (;;) { // タスクAが消してくれるまでスリープ
        task_sleep(task);
    }
}

// startコマンド
void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal) {
    struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
    struct SHEET *sht = open_console(shtctl, memtotal);
    struct FIFO32 *fifo = &sht->task->fifo;
    int i;
    sheet_slide(sht, 32, 4);
    sheet_updown(sht, shtctl->top);
    // コマンドラインに入力された文字列を新しいコンソールに一文字ずつ入力
    for (i = 6; cmdline[i] != 0; i++) {
        fifo32_put(fifo, cmdline[i] + 256);
    }
    fifo32_put(fifo, 10 + 256); // Enter
    cons_newline(cons);
    return;
}

// ncstコマンド
void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal) {
    struct TASK *task = open_constask(0, memtotal);
    struct FIFO32 *fifo = &task->fifo;
    int i;
    // コマンドラインに入力された文字列を新しいコンソールに一文字ずつ入力
    for (i = 5; cmdline[i] != 0; i++) {
        fifo32_put(fifo, cmdline[i] + 256);
    }
    fifo32_put(fifo, 10 + 256); // Enter
    cons_newline(cons);
    return;
}

// ファイル名からアプリを実行
int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct FILEINFO *finfo;
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    struct SHTCTL *shtctl;
    struct SHEET *sht;
    char name[13], *p, *q;
    struct TASK *task = task_now(); // いま動いてるタスクの番地
    int segsiz, datsiz, esp, dathrb;
    int i;

    // コマンドラインからファイル名を作る
    for (i = 0; i < 8; i++) {
        if (cmdline[i] <= ' ') {
            break;
        }
        name[i] = cmdline[i];
    }
    name[i] = 0;    // 最後の文字を0にする

    finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    // if (finfo == 0 && name[i - 1] != '.') {
    if (finfo == 0) {
        // ファイルが見つからなかったら".hrb"をつけてもう一度ためす
        name[i    ] = '.';
        name[i + 1] = 'H';
        name[i + 2] = 'R';
        name[i + 3] = 'B';
        name[i + 4] = 0;
        finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    }

    if (finfo != 0) {
        // ファイルが見つかったとき
        p = (char *) memman_alloc_4k(memman, finfo->size);
        // cmd_appからhrb_apiにコードセグメントがどこから始まったか伝える, 空いているとこを使う
        // *((int *) 0xfe8) = (int) p;
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
        if(finfo->size >= 36 && _strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
            segsiz = *((int *) (p + 0x0000)); // データセグメントの大きさ
            esp    = *((int *) (p + 0x000c)); // 初期esp
            datsiz = *((int *) (p + 0x0010)); // データセクションからデータセグメントにコピーする大きさ
            dathrb = *((int *) (p + 0x0014)); // hrbファイル内のデータセクションの位置
            // データセグメントのサイズに基づいてメモリ確保
            q = (char *) memman_alloc_4k(memman, segsiz);
            task->ds_base = (int) q;
            // 0x60 : アプリのセグメントだと伝える
            // コードセグメント, タスク内のLDTを用いる
            set_segmdesc(task->ldt + 0, finfo->size - 1, (int) p, AR_CODE32_ER + 0x60);
            // データセグメント，タスク内のLDTを用いる
            set_segmdesc(task->ldt + 1, segsiz - 1,      (int) q, AR_DATA32_RW + 0x60);
            for (i = 0; i < datsiz; i++) {
                q[esp + i] = p[dathrb + i];
            }
            start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));
            shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
            for (i = 0; i < MAX_SHEETS; i++) {
                sht = &(shtctl->sheets0[i]);
                if ((sht->flags & 0x11) == 0x11 && sht->task == task) {
                    // アプリが開きっぱなしの下敷きがあったとき
                    sheet_free(sht);    // 閉じる
                }
            }

            for (i = 0; i < 8; i++) {
                // クローズしていないファイルをクローズする
                if (task->fhandle[i].buf != 0) {
                    memman_free_4k(memman, (int) task->fhandle[i].buf, task->fhandle[i].size);
                    task->fhandle[i].buf = 0;
                }
            }
            timer_cancelall(&task->fifo);
            memman_free_4k(memman, (int) q, segsiz);
        } else {
            cons_putstr0(cons, ".hrb file format error.\n");
        }
        memman_free_4k(memman, (int) p, finfo->size);
        cons_newline(cons);
        return 1;
    }
    return 0;
}

// 文字コード0がくるまで文字列を表示する
void cons_putstr0(struct CONSOLE *cons, char *s) {
    for (; *s != 0; s++) {
        cons_putchar(cons, *s, 1);
    }
    return;
}

// 指定した文字数分文字列を表示する
void cons_putstr1(struct CONSOLE *cons, char *s, int l) {
    int i;
    for (i = 0; i < l; i++) {
        cons_putchar(cons, s[i], 1);
    }
    return;
}

// API処理プログラム
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax) {
    int i;
    struct TASK *task = task_now();
    int ds_base = task->ds_base;
    struct CONSOLE *cons = task->cons;
    struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);    // shtctlの値をHariMainからもらう
    struct SHEET *sht;
    struct FIFO32 *sys_fifo = (struct FIFO32 *) *((int *) 0x0fec);
    struct FILEINFO *finfo;
    struct FILEHANDLE *fh;
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

    int *reg = &eax + 1;    // eaxの次の番地，アプリに値を返すために用いる
        // 保存のためのPUSHADを強引に書き換える
        // reg[0] : EDIx
        // reg[1] : ESI
        // reg[2] : EBP
        // reg[3] : ESP
        // reg[4] : EBX
        // reg[5] : EDX
        // reg[6] : ECX
        // reg[7] : EAX

    if (edx == 1) {
        cons_putchar(cons, eax & 0xff, 1);
    } else if (edx == 2) {
        cons_putstr0(cons, (char *) ebx + ds_base);
    } else if (edx == 3) {
        cons_putstr1(cons, (char *) ebx + ds_base, ecx);
    } else if (edx == 4) {
        return &(task->tss.esp0);
    } else if (edx == 5) {
        // ウィンドウを開くAPI
        sht = sheet_alloc(shtctl);
        sht->task = task;
        sht->flags |= 0x10; // 自動クローズを有効にする
        sheet_setbuf(sht, (char *) ebx + ds_base, esi, edi, eax);
        make_window8((char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
        sheet_slide(sht, ((shtctl->xsize - esi) / 2) & ~3, (shtctl->ysize - edi) / 2); // 4の倍数になるように0x1..100と&演算
        sheet_updown(sht, shtctl->top - 1); // マウスと同じ高さになるように指定する->マウスはこの上に乗る
        reg[7] = (int) sht;
    } else if (edx == 6) {
        // ウィンドウに文字を表示するAPI
        sht = (struct SHEET *) (ebx & 0xfffffffe);
        putfont8_asc(sht->buf, sht->bxsize, esi, edi, eax, (char *) ebp + ds_base);
        if ((ebx & 1) == 0) {
            sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
        }
    } else if (edx == 7) {
        // ウィンドウに四角をつくるAPI
        /*
         * %ebx win
         * %eax x0
         * %ecx y0
         * %esi x1
         * %edi y1
         * %ebp col
         * */
        sht = (struct SHEET *) (ebx & 0xfffffffe);
        boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
        if ((ebx & 1) == 0) {
            sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
        }
    } else if (edx == 8) {
        /*
         * ebx : memman
         * eax : malloc開始アドレス（管理アドレスの最初）
         * ecx : 管理させる領域のバイト数
         * */
        memman_init((struct MEMMAN *) (ebx + ds_base));
        ecx &= 0xfffffff0; // 16byte単位
        memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
    } else if (edx == 9) {
        /*
         * ebx : memman
         * ecx : 要求バイト数
         * eax : 確保した領域のアドレス（戻り値）
         */
        ecx = (ecx + 0x0f) & 0xfffffff0; // 16byte単位に切り上げ
        reg[7] = memman_alloc((struct MEMMAN *) (ebx + ds_base), ecx);
    } else if (edx == 10) {
        /*
         * ebx : memman
         * ebx : 開放する領域のアドレス
         * ecx : 開放したいバイト数
         */
        ecx = (ecx + 0x0f) & 0xfffffff0; // 16byte単位に切り上げ
        memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
    } else if (edx == 11) {
        /* ウィンドウに点を打つ
         * ebx : ウィンドウの番号
         * esi : 表示位置のx座標
         * edi : 表示位置のy座標
         * eax : 色番号
         */
        sht = (struct SHEET *) (ebx & 0xfffffffe);
        sht->buf[sht->bxsize * edi + esi] = eax;
        if ((ebx & 1) == 0) {
            sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
        }
    } else if (edx == 12) {
        /* ウィンドウのリフレッシュ
         * ebx : ウィンドウの番号
         * eax : x0
         * ecx : y0
         * esi : x1
         * edi : y1
         */
        sht = (struct SHEET *) ebx;
        sheet_refresh(sht, eax, ecx, esi, edi);
    } else if (edx == 13) {
        /* 線を引く
         * ebx : ウィンドウの番号
         * eax : x0
         * ecx : y0
         * esi : x1
         * edi : y1
         * ebp : col
         */
        sht = (struct SHEET *) (ebx & 0xfffffffe);
        hrb_api_linewin(sht, eax, ecx, esi, edi, ebp);
        if ((ebx & 1) == 0) {
            sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
        }
    } else if (edx == 14) {
        /* ウィンドウのクローズ
         * ebx : ウィンドウの番号
         */
        sheet_free((struct SHEET *) ebx);
    } else if (edx == 15) {
        /* ウィンドウのクローズ
         * ebx : ウィンドウの番号
         */
        for (;;) {
            io_cli();
            if (fifo32_status(&task->fifo) == 0) {
                if (eax != 0) {
                    task_sleep(task);   // FIFOがからのときは寝る
                } else {
                    io_sti();
                    reg[7] = -1;
                    return 0;
                }
            }
            i = fifo32_get(&task->fifo);
            io_sti();
            if (i <= 1) {
                // カーソル用タイマ
                // アプリ実行中はカーソルア出ないのでいつも次の表示用の1を注文
                timer_init(cons->timer, &task->fifo, 1);
                timer_settime(cons->timer, 50);
            }
            if (i == 2) {
                // カーソルON
                cons->cur_c = COL8_FFFFFF;
            }
            if (i == 3) {
                // カーソルOFF
                cons->cur_c = -1;
            }
            if (i == 4) {
                // コンソールだけを閉じる
                timer_cancel(cons->timer);
                io_cli();
                fifo32_put(sys_fifo, cons->sht - shtctl->sheets0 + 2024); // 2024~2279
                cons->sht = 0;
                io_sti();
            }
            if (i >= 256) {
                // キーボードデータ
                reg[7] = i - 256;
                return 0;
            }
        }
    } else if (edx == 16) {
        reg[7] = (int) timer_alloc();
        ((struct TIMER *) reg[7])->flags2 = 1;  // API呼び出して作ったタイマは自動削除を有効化
    } else if (edx == 17) {
        /* タイマの送信データ設定
         * ebx : タイマ番号
         * eax : 出0田
         */
        // eax がアプリにFIFOデータを渡すときに-256されるので足しておく
        timer_init((struct TIMER *) ebx, &task->fifo, eax + 256);
    } else if (edx == 18) {
        /* タイマの時間設定
         * ebx : タイマ番号
         * eax : 時間
         */
        timer_settime((struct TIMER *) ebx, eax);
    } else if (edx == 19) {
        /* タイマの解放
         * ebx : タイマ番号
         */
        timer_free((struct TIMER *) ebx);
    } else if (edx == 20) {
        /* アラームを鳴らす
         * eax : 周波数
         */
        if (eax == 0) {
            i = io_in8(0x61); // BEEPのON/OFF
            io_out8(0x61, i & 0x0d); // OFF
        } else {
            i = 1193180000 / eax;
            io_out8(0x43, 0xb6);
            io_out8(0x42, i & 0xff); // 設定上位8bit
            io_out8(0x42, i >> 8); // 設定下位8bit
            i = io_in8(0x61); // BEEPのON/OFF
            io_out8(0x61, (i | 0x03) & 0x0f); // ON
        }
    } else if (edx == 21) {
        /*
         * ファイルのオープン
         * EBX : ファイル名
         * EAX : ファイルハンドル(OSが返す値，0なら失敗)
         */
        for (i = 0; i < 8; i++) {
            if (task->fhandle[i].buf == 0) {
                break;
            }
        }
        fh = &task->fhandle[i];
        reg[7] = 0;
        if (i < 8) {
            finfo = file_search((char *) ebx + ds_base, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
            if (finfo != 0) {
                reg[7] = (int) fh;
                fh->buf = (char *) memman_alloc_4k(memman, finfo->size);
                fh->size = finfo->size;
                fh->pos = 0;
                file_loadfile(finfo->clustno, finfo->size, fh->buf, task->fat, (char *) (ADR_DISKIMG + 0x003e00));
            }
        }
    } else if (edx == 22) {
        /*
         * ファイルクローズ
         * eax : ファイルハンドル
         */
        fh = (struct FILEHANDLE *) eax;
        memman_free_4k(memman, (int) fh->buf, fh->size);
        fh->buf = 0;
    } else if (edx == 23) {
        /*
         * ファイルのシーク
         * eax : ファイルハンドル
         * ecx : シークモード
         *  0 : シーク原点はファイルの先頭
         *  1 : シーク原点は現在のアクセス位置
         *  2 : シーク原点はファイルの終端
         * ebx : シーク量
         */
        fh = (struct FILEHANDLE *) eax;
        if (ecx == 0) {
            fh->pos = ebx;
        } else if (ecx == 1) {
            fh->pos += ebx;
        } else if (ecx == 2) {
            fh->pos = fh->size + ebx;
        }
        if (fh->pos < 0) {
            fh->pos = 0;
        }
        if (fh->pos > fh->size) {
            fh->pos = fh->size;
        }
    } else if (edx == 24) {
        /*
         * ファイルサイズの取得
         * eax : ファイルハンドル
         * ecx : ファイルサイズ取得モード
         *  0 : 普通のファイルサイズ
         *  1 : 現在の読み込み位置がファイルから何バイト目か
         *  2 : ファイルの終端からみた現在位置までのバイト数
         * eax : ファイルサイズ(OSから返される)
         */
        fh = (struct FILEHANDLE *) eax;
        if (ecx == 0) {
            reg[7] = fh->size;
        } else if (ecx == 1) {
            reg[7] = fh->pos;
        } else if (ecx == 2) {
            reg[7] = fh->pos - fh->size;
        }
    } else if (edx == 25) {
        /*
         * ファイルの読み込み
         * eax : ファイルハンドリング
         * ebx : バッファ番地
         * ecx : 最大読み込めたバイト数
         * eax : 今回読み込めたバイト数
         */
        fh = (struct FILEHANDLE *) eax;
        for (i = 0; i < ecx; i++) {
            if (fh->pos == fh->size) {
                break;
            }
            *((char *) ebx + ds_base + i) = fh->buf[fh->pos];
            fh->pos++;
        }
        reg[7] = i;
    } else if (edx == 26) {
        /*
         * コマンドラインの取得
         * ebx : コマンドラインを格納する番地
         * ecx : 何バイトまで格納できるか
         * eax : 何バイト格納したか(OSから返される)
         */
        i = 0;
        for (;;) {
            *((char *) ebx + ds_base + i) = task->cmdline[i];
            if (task->cmdline[i] == 0) {
                break;
            }
            if (i >= ecx) {
                break;
            }
            i++;
        }
        reg[7] = i;
    }
    return 0;
}

int *inthandler0c(int *esp) {
    struct TASK *task = task_now();
    struct CONSOLE *cons = task->cons;
    char s[30];
    cons_putstr0(cons, "\nINT 0c :\n Stack Exception.\n");
    sprintf(s, "EIP = %d\n", esp[11]);
    cons_putstr0(cons, s);
    return &(task->tss.esp0); // 異常終了
}

int *inthandler0d(int *esp) {
    struct TASK *task = task_now();
    struct CONSOLE *cons = task->cons;
    char s[30];
    cons_putstr0(cons, "\nINT 0d :\n Genelar Protected Exception.\n");
    sprintf(s, "EIP = %d\n", esp[11]);
    cons_putstr0(cons, s);
    return &(task->tss.esp0); // 異常終了
}

/* 線を引く
 * ebx : ウィンドウの番号
 * eax : x0
 * ecx : y0
 * esi : x1
 * edi : y1
 * ebp : col
 */
void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col) {
    int i, x, y, len, dx, dy;

    dx = x1 - x0;
    dy = y1 - y0;
    x = x0 << 10; // 1024で割る
    y = y0 << 10; // 1024で割る
    if (dx < 0) {
        dx = -dx;
    }
    if (dy < 0) {
        dy = -dy;
    }
    // lenを決める，x, y大きいほうを選ぶ
    if (dx >= dy) {
        len = dx + 1;
        if (x0 > x1) {
            dx = -1024; // 1ずつ進める
        } else {
            dx =  1024;
        }
        if (y0 <= y1) {
            dy = ((y1 - y0 + 1) << 10) / len; // 線を滑らかに変化させる
        } else {
            dy = ((y1 - y0 - 1) << 10) / len;
        }
    } else {
        len = dy + 1;
        if (y0 > y1) {
            dy = -1024;
        } else {
            dy =  1024;
        }
        if (x0 <= x1) {
            dx = ((x1 - x0 + 1) << 10) / len;
        } else {
            dx = ((x1 - x0 - 1) << 10) / len;
        }
    }
    for (i = 0; i < len; i++) {
        sht->buf[(y >> 10) * sht->bxsize + (x >> 10)] = col;
        x += dx;
        y += dy;
    }
    return;
}