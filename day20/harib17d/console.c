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
    } else if (_strcmp(cmdline, "hlt") == 0) {
        cmd_hlt(cons, fat);
    } else if (cmdline[0] != 0) {
        // コマンドはない & 空行ではない
        putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, "Bad command", 12);
        cons_newline(cons);
        cons_newline(cons);
    }
    return;
}

// memコマンド
void cmd_mem(struct CONSOLE *cons, unsigned int memtotal) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    char s[30];

   sprintf(s, "total %dMB", memtotal / (1024 * 1024));
   putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 30);
   cons_newline(cons);
   sprintf(s, "free  %dKB", memman_total(memman) / 1024);
   putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 30);
   cons_newline(cons);
   cons_newline(cons);
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
                sprintf(s, "filename.ext   %d", finfo[x].size);
                for (y = 0; y < 8; y++) {
                    s[y] = finfo[x].name[y];
                }
                s[ 9] = finfo[x].ext[0];
                s[10] = finfo[x].ext[1];
                s[11] = finfo[x].ext[2];
                putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 30);
                cons_newline(cons);
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
        for (i = 0; i < finfo->size; i++) {
            cons_putchar(cons, p[i], 1);
        }
        memman_free_4k(memman, (int) p, finfo->size);
    } else {
        // ファイルが見つからなかった
        putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
        cons_newline(cons);
    }
    cons_newline(cons);
    return;
}

// hlt.asmのアプリケーションを起動
void cmd_hlt(struct CONSOLE *cons, int *fat) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct FILEINFO *finfo = file_search("HLT.HRB", (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    char *p;

    if (finfo != 0) {
        // ファイルが見つかったとき
        p = (char *) memman_alloc_4k(memman, finfo->size);
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
        set_segmdesc(gdt + 1003, finfo->size - 1, (int) p, AR_CODE32_ER);
        farcall(0, 1003 * 8);
        memman_free_4k(memman, (int) p, finfo->size);
    } else {
        // ファイルが見つからなかった
        putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
        cons_newline(cons);
    }
    cons_newline(cons);
    return;
}

