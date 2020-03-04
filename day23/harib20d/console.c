#include "bootpack.h"

void console_task(struct SHEET *sheet, unsigned int memtotal) {
    struct TIMER *timer;
    struct TASK *task = task_now();   // 自分自身をつかっていないときにスリープさせる -> アドレス欲しい
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    int i, fifobuf[128], *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
    struct CONSOLE cons;
    char cmdline[30];
    cons.sht = sheet;
    cons.cur_x =  8;
    cons.cur_y = 28;
    cons.cur_c = -1;
    *((int *) 0x0fec) = (int) &cons;    // コンソールの番地をメモリに記録しておく

    fifo32_init(&task->fifo, 128, fifobuf, task);
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);
    file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));

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
            if (i <= 1) {   // カーソル用タイマ
                if (i != 0) {
                    timer_init(timer, &task->fifo, 0);    // 次は0にする
                    if (cons.cur_c >= 0) {
                        cons.cur_c = COL8_FFFFFF;
                    }
                } else {
                    timer_init(timer, &task->fifo, 1);    // 次は1にする
                    if (cons.cur_c >= 0) {
                        cons.cur_c = COL8_000000;
                    }
                }
                timer_settime(timer, 50);
            }
            if (i == 2) {
                // カーソルON
                cons.cur_c = COL8_FFFFFF;
            }
            if (i == 3) {
                // カーソルOFF
                boxfill8(sheet->buf, sheet->bxsize, COL8_000000, \
                    cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
                cons.cur_c = -1;
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
            if (cons.cur_c >= 0) {
                boxfill8(sheet->buf, sheet->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
            }
            sheet_refresh(sheet, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
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
            putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
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
        putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
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
    } else if (_strncmp(cmdline, "type ", 5) == 0) {
        cmd_type(cons, fat, cmdline);
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

// typeコマンド
void cmd_type(struct CONSOLE *cons, int *fat, char *cmdline) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct FILEINFO *finfo = file_search(cmdline + 5, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    char *p;
    int i;

    if (finfo != 0) {
        // ファイルが見つかったとき
        p = (char *) memman_alloc_4k(memman, finfo->size);
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
        cons_putstr1(cons, p, finfo->size);
        memman_free_4k(memman, (int) p, finfo->size);
    } else {
        // ファイルが見つからなかった
        cons_putstr0(cons, "File not found\n");
    }
    cons_newline(cons);
    return;
}

// ファイル名からアプリを実行
int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct FILEINFO *finfo;
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    char name[18], *p, *q;
    struct TASK *task = task_now(); // いま動いてるタスクの番地
    int segsiz, datsiz, esp, dathrb;
    int i;

    // コマンドラインからファイル名を作る
    for (i = 0; i < 13; i++) {
        if (cmdline[i] <= ' ') {
            break;
        }
        name[i] = cmdline[i];
    }
    name[i] = 0;    // 最後の文字を0にする

    finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    if (finfo == 0 && name[i - 1] != '.') {
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
        *((int *) 0xfe8) = (int) p;
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
        if(finfo->size >= 36 && _strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
            segsiz = *((int *) (p + 0x0000)); // データセグメントの大きさ
            esp    = *((int *) (p + 0x000c)); // 初期esp
            datsiz = *((int *) (p + 0x0010)); // データセクションからデータセグメントにコピーする大きさ
            dathrb = *((int *) (p + 0x0014)); // hrbファイル内のデータセクションの位置
            // データセグメントのサイズに基づいてメモリ確保
            q = (char *) memman_alloc_4k(memman, segsiz);
            *((int *) 0xfe8) = (int) q;
            set_segmdesc(gdt + 1003, finfo->size - 1, (int) p, AR_CODE32_ER + 0x60);    // アクセス権に0x60を足す
            set_segmdesc(gdt + 1004, segsiz - 1,      (int) q, AR_DATA32_RW + 0x60);    // アプリ実行中なことをCPUが理解
            for (i = 0; i < datsiz; i++) {
                q[esp + i] = p[dathrb + i];
            }
            start_app(0x1b, 1003 * 8, esp, 1004 * 8, &(task->tss.esp0));
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
    int ds_base = *((int *) 0xfe8);
    struct TASK *task = task_now();
    struct CONSOLE *cons = (struct CONSOLE *) *((int *) 0x0fec);
    struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);    // shtctlの値をHariMainからもらう
    struct SHEET *sht;
    int *reg = &eax + 1;    // eaxの次の番地，アプリに値を返すために用いる
        // 保存のためのPUSHADを強引に書き換える
        // reg[0] : EDI
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
        sht = sheet_alloc(shtctl);
        sheet_setbuf(sht, (char *) ebx + ds_base, esi, edi, eax);
        make_window8((char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
        sheet_slide(sht, 100, 50);
        sheet_updown(sht, 3); // task_aのうえにshtを置く
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
    }
    return 0;
}

int *inthandler0c(int *esp) {
    struct CONSOLE *cons = (struct CONSOLE *) *((int *) 0x0fec);
    struct TASK *task = task_now();
    char s[30];
    cons_putstr0(cons, "\nINT 0c :\n Stack Exception.\n");
    sprintf(s, "EIP = %d\n", esp[11]);
    cons_putstr0(cons, s);
    return &(task->tss.esp0); // 異常終了
}

int *inthandler0d(int *esp) {
    struct CONSOLE *cons = (struct CONSOLE *) *((int *) 0x0fec);
    struct TASK *task = task_now();
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
        if (x <= x1) {
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