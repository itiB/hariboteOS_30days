#include "bootpack.h"

// 改行する，一番下まで言ったらスクロールする
int cons_newline(int cursor_y, struct SHEET *sheet) {
    int x, y;
    if (cursor_y < 28 + 112) {
        // 次の行へ
        cursor_y += 16;
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
    return cursor_y;
}

void console_task(struct SHEET *sheet, unsigned int memtotal) {
    struct TIMER *timer;
    struct TASK *task = task_now();   // 自分自身をつかっていないときにスリープさせる -> アドレス欲しい
    int i, fifobuf[128], cursor_x = 16, cursor_c = -1, cursor_y = 28;
    char s[2], cmdline[30], *p;
    int x, y;
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
    int *fat = (int *) memman_alloc_4k(memman, 4 * 2880);

    fifo32_init(&task->fifo, 128, fifobuf, task);
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);

    file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));

    // プロンプトの表示
    putfonts8_asc_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, ">", 1);

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
                    if (cursor_c >= 0) {
                        cursor_c = COL8_FFFFFF;
                    }
                } else {
                    timer_init(timer, &task->fifo, 1);    // 次は1にする
                    if (cursor_c >= 0) {
                        cursor_c = COL8_000000;
                    }
                }
                timer_settime(timer, 50);
            }
            if (i == 2) {
                // カーソルON
                cursor_c = COL8_FFFFFF;
            }
            if (i == 3) {
                // カーソルOFF
                boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cursor_x, cursor_y, cursor_x + 7, 43);
                cursor_c = -1;
            }
            if (256 <= i && i <= 511) { // キーボードデータ
                if (i == 8 + 256) {
                    // バックスペースなら
                    if (cursor_x > 16) {
                        // カーソルをスペースでけしてカーソルを戻す
                        putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
                        cursor_x -= 8;
                    }
                } else if (i == 10 + 256) {
                    // Enter
                    // まずはカーソルを消す
                    putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
                    cmdline[cursor_x / 8 - 2] = 0;  // 入力された文字を初期化
                    cursor_y = cons_newline(cursor_y, sheet);
                    // コマンド実行
                    if (_strcmp(cmdline, "mem") == 0) {
                    // if (cmdline[0] == 'm' && cmdline[1] == 'e' && cmdline[2] == 'm' && cumdline[3] == 0) {
                        // memコマンド
                        sprintf(s, "total %dMB", memtotal / (1024 * 1024));
                        putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                        cursor_y = cons_newline(cursor_y, sheet);
                        sprintf(s, "free  %dKB", memman_total(memman) / 1024);
                        putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                        cursor_y = cons_newline(cursor_y, sheet);
                        cursor_y = cons_newline(cursor_y, sheet);
                    } else if (_strcmp(cmdline, "cls") == 0) {
                        for (y = 28; y < 28 + 128; y++) {
                            for (x = 8; x < 8 + 240; x++) {
                                sheet->buf[x + y * sheet->bxsize] = COL8_000000;
                            }
                        }
                        sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
                        cursor_y = 28;
                    } else if (_strcmp(cmdline, "ls") == 0) {
                        // lsコマンド
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
                                    putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                                    cursor_y = cons_newline(cursor_y, sheet);
                                }
                            }
                        }
                        cursor_y = cons_newline(cursor_y, sheet);
                    // } else if (cmdline[0] == 't' && cmdline[1] == 'y'&& cmdline[2] == 'p' \
                        && cmdline[3] == 'e' && cmdline[4] == ' ') {
                    } else if (_strncmp(cmdline, "type", 5) == 0) {
                        // file名を初期化
                        for (y = 0; y < 11; y++) { s[y] = ' '; }
                        y = 0;
                        for(x = 5; y < 11 && cmdline[x] != 0; x++) {
                            if (cmdline[x] == '.' && y <= 8) {
                                y = 8;
                            } else {
                                s[y] = cmdline[x];
                                if ('a' <= s[y] && s[y] <= 'z') {
                                    // 小文字を大文字に直す
                                    s[y] -= 0x20;
                                }
                                y++;
                            }
                        }
                        // ファイルを探す
                        for (x = 0; x < 244; ) {
                            if (finfo[x].name[0] == 0x00) {
                                break;
                            }
                            if ((finfo[x].type & 0x18) == 0) {
                                for (y = 0; y < 11; y++) {
                                    if (finfo[x].name[y] != s[y]) {
                                        goto type_next_file;
                                    }
                                }
                                break;  // ファイルが見つかった
                            }
type_next_file:
                            x++;
                        }
                        if (x < 224 && finfo[x].name[0] != 0x00) {
                            // ファイルが見つかったとき
                            p = (char *) memman_alloc_4k(memman, finfo[x].size);
                            file_loadfile(finfo[x].clustno, finfo[x].size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
                            // y = finfo[x].size;
                            // p = (char *) (finfo[x].clustno * 512 + 0x003e00 + ADR_DISKIMG);
                            cursor_x = 8;
                            for (y = 0; y < finfo[x].size; y++) {
                                s[0] = p[y];
                                s[1] = 0;
                                if (s[0] == 0x09) {
                                    // Tab
                                    for (;;) {
                                        putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
                                        cursor_x += 8;
                                        if(cursor_x == 8 + 240){
                                            cursor_x = 8;
                                            cursor_y = cons_newline(cursor_y, sheet);
                                        }
                                        if (((cursor_x - 8) & 0x1f) == 0) { // コンソールの枠分(-8)
                                            break;  // 32で割り切れたらBreak
                                        }
                                    }
                                } else if (s[0] == 0x0a) {
                                    // 改行
                                    cursor_x = 8;
                                    cursor_y = cons_newline(cursor_y, sheet);
                                } else if (s[0] == 0x0d) {
                                    // 復帰
                                    // とりあえずなにもしない
                                } else {
                                    putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
                                    cursor_x += 8;
                                    if (cursor_x == 8 + 240) {
                                        // 右端まで来たら改行
                                        cursor_y = cons_newline(cursor_y, sheet);
                                        cursor_x = 8;
                                    }
                                }
                            }
                            memman_free_4k(memman, (int) p, finfo[x].size);
                        } else {
                            // ファイルが見つからなかった
                            putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
                            cursor_y = cons_newline(cursor_y, sheet);
                        }
                        cursor_y = cons_newline(cursor_y, sheet);
                    } else if (cmdline[0] != 0) {
                        // コマンドはない & 空行ではない
                        putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "Bad command", 12);
                        cursor_y = cons_newline(cursor_y, sheet);
                        cursor_y = cons_newline(cursor_y, sheet);
                    }
                    // プロンプトの表示
                    putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, ">", 1);
                    cursor_x = 16;
                } else {
                    // 一般文字
                    if (cursor_x < 240) {
                        // 一般文字を表示してからカーソルを進める
                        s[0] = i - 256;
                        s[1] = 0;
                        cmdline[cursor_x / 8 - 2] = i - 256;
                        putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
                        cursor_x += 8;
                    }
                }
            }
            // カーソルの再表示
            if (cursor_c >= 0) {
                boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
            }
            sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
        }
    }
}
