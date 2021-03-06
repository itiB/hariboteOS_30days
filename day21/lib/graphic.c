#include "bootpack.h"

void set_palette(int start, int end, unsigned char *rgb) {
    int i, eflags;

    eflags = io_load_eflags();
    io_cli();   // アクセス中に割り込みが入らないようにする(割り込みフラグCLIを0にする)

    io_out8(0x03c8, start);
    for (i = start; i <= end; i++) {
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }

    io_store_eflags(eflags);    // 割り込みフラグを1に戻す
    return;
}

void init_palette(void) {
    // static char: データにしか使えないDB命令みたいなことができる
    //  ex. DB 0x00, 0x00, 0x00, 0xff...
    // unsigned: 符号のない値として使ってね
    static unsigned char table_rgb[16 * 3] = {
        0x00, 0x00, 0x00,   //  0: 黒
        0xff, 0x00, 0x00,   //  1: 明るい赤
        0x00, 0xff, 0x00,   //  2: 明るい緑
        0xff, 0xff, 0x00,   //  3: 明るい黄色
        0x00, 0x00, 0xff,   //  4: 明るい青
        0xff, 0x00, 0xff,   //  5: 明るい紫
        0x00, 0xff, 0xff,   //  6: 明るい水色
        0xff, 0xff, 0xff,   //  7: 白
        0xc6, 0xc6, 0xc6,   //  8: 明るい灰色
        0x84, 0x00, 0x00,   //  9: 暗い赤
        0x00, 0x84, 0x00,   // 10: 暗い緑
        0x84, 0x84, 0x00,   // 11: 暗い黄色
        0x00, 0x00, 0x84,   // 12: 暗い青
        0x84, 0x00, 0x84,   // 13: 暗い紫
        0x00, 0x84, 0x84,   // 14: 暗い水色
        0x84, 0x84, 0x84    // 15: 暗い灰色
    };

    set_palette(0, 15, table_rgb);
    return;
}

void putblock8_8(char *vram, int vxsize, int pxsize,
    int pysize, int px0, int py0, char *buf, int bxsize) {
    int x, y;

    for(y = 0; y < pysize; y++) {
        for (x = 0; x < pxsize; x++) {
            vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
        }
    }
    return;
}

void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1) {
    // 0xa0000 + x + y * 320
    // 開始番地(画面左上) + (x, y)
    int x, y;

    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++) {
            vram[y * xsize + x] = c;
        }
    }
    return;
}


void init_screen(unsigned char *vram, int xsize, int ysize) {
    boxfill8(vram, xsize, COL8_008484,  0,         0,          xsize -  1, ysize - 29);
    boxfill8(vram, xsize, COL8_C6C6C6,  0,         ysize - 28, xsize -  1, ysize - 28);
    boxfill8(vram, xsize, COL8_FFFFFF,  0,         ysize - 27, xsize -  1, ysize - 27);
    boxfill8(vram, xsize, COL8_C6C6C6,  0,         ysize - 26, xsize -  1, ysize -  1);

    boxfill8(vram, xsize, COL8_FFFFFF,  3,         ysize - 24, 59,         ysize - 24);
    boxfill8(vram, xsize, COL8_FFFFFF,  2,         ysize - 24,  2,         ysize -  4);
    boxfill8(vram, xsize, COL8_848484,  3,         ysize -  4, 59,         ysize -  4);
    boxfill8(vram, xsize, COL8_848484, 59,         ysize - 23, 59,         ysize -  5);
    boxfill8(vram, xsize, COL8_000000,  2,         ysize -  3, 59,         ysize -  3);
    boxfill8(vram, xsize, COL8_000000, 60,         ysize - 24, 60,         ysize -  3);

    boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 24, xsize -  4, ysize - 24);
    boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 23, xsize - 47, ysize -  4);
    boxfill8(vram, xsize, COL8_FFFFFF, xsize - 47, ysize -  3, xsize -  4, ysize -  3);
    boxfill8(vram, xsize, COL8_FFFFFF, xsize -  3, ysize - 24, xsize -  3, ysize -  3);
    return;
}

void putfont8(char *vram, int xsize, int x, int y, char c, char *font) {
    int i;
    char *p, d;     // data

    for (i = 0; i < 16; i++) {
        p = vram + (y + i) * xsize + x;
        d = font[i];
        if ((d & 0x80) != 0) { p[0] = c; }
        if ((d & 0x40) != 0) { p[1] = c; }
        if ((d & 0x20) != 0) { p[2] = c; }
        if ((d & 0x10) != 0) { p[3] = c; }
        if ((d & 0x08) != 0) { p[4] = c; }
        if ((d & 0x04) != 0) { p[5] = c; }
        if ((d & 0x02) != 0) { p[6] = c; }
        if ((d & 0x01) != 0) { p[7] = c; }
    }
    return;
}

void putfont8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s) {
    extern char hankaku[4096];     // ../hankaku.c の中にある
    for (; *s != 0x00; s++) {       // 文字列の最後(0x00)まで回す
        putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
        x += 8;
    }
    return;
}

void init_mouse_cursor8(char *mouse, char bc) {
    static char cursor[16][16] = {
        "**************..",
        "*ooooooooooo*...",
        "*oooooooooo*....",
        "*ooooooooo*.....",
        "*oooooooo*......",
        "*ooooooo*.......",
        "*ooooooo*.......",
        "*oooooooo*......",
        "*oooo**ooo*.....",
        "*ooo*..*ooo*....",
        "*oo*....*ooo*...",
        "*o*......*ooo*..",
        "**........*ooo*.",
        "*..........*ooo*",
        "............*oo*",
        ".............***"
    };
    int x, y;

    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            if (cursor[y][x] == '*') {
                mouse[y * 16 + x] = COL8_000000;
            }
            if (cursor[y][x] == 'o') {
                mouse[y * 16 + x] = COL8_FFFFFF;
            }
            if (cursor[y][x] == '.') {
                mouse[y * 16 + x] = bc;     // back-color
            }
        }
    }
    return;
}

// 画面にウィンドウを表示させる
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act) {
    boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         xsize - 1, 0        );
    boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         xsize - 2, 1        );
    boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         0,         ysize - 1);
    boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         1,         ysize - 2);
    boxfill8(buf, xsize, COL8_848484, xsize - 2, 1,         xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, xsize - 1, 0,         xsize - 1, ysize - 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 2,         2,         xsize - 3, ysize - 3);
    boxfill8(buf, xsize, COL8_848484, 1,         ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, 0,         ysize - 1, xsize - 1, ysize - 1);
    make_wtitle8(buf, xsize, title, act);
    return;
}

void make_wtitle8(unsigned char *buf, int xsize, char *title, char act) {
    static char closebtn[14][16] =  {
        "ooooooooooooooo@",
        "oQQQQQQQQQQQQQ$@",
        "oQQQQQQQQQQQQQ$@",
        "oQQQ@@QQQQ@@QQ$@",
        "oQQQQ@@OO@@QQQ$@",
        "oQQQQQ@@@@QQQQ$@",
        "oQQQQQQ@@QQQQQ$@",
        "oQQQQQ@@@@QQQQ$@",
        "oQQQQ@@OO@@QQQ$@",
        "oQQQ@@QQQQ@@QQ$@",
        "oQQQQQQQQQQQQQ$@",
        "oQQQQQQQQQQQQQ$@",
        "o$$$$$$$$$$$$$$@",
        "@@@@@@@@@@@@@@@@"
    };
    int x, y;
    char c, tc, tbc;

    if (act != 0) {
        tc  = COL8_FFFFFF;
        tbc = COL8_000084;
    } else {
        tc  = COL8_C6C6C6;
        tbc = COL8_848484;
    }
    boxfill8(buf, xsize, tbc, 3, 3, xsize - 4, 20);
    putfont8_asc(buf, xsize, 24, 4, tc, title);

    for (y = 0; y < 14; y++) {
        for (x = 0; x < 16; x++) {
            c = closebtn[y][x];
            if      (c == '@') { c = COL8_000000; }
            else if (c == '$') { c = COL8_848484; }
            else if (c == 'Q') { c = COL8_C6C6C6; }
            else               { c = COL8_FFFFFF; }
            buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
        }
    }
}
