extern void io_hlt(void);

void sprintf (char *str, char *fmt, ...);

void init_palette(void);
void init_screen(unsigned char *vram, int xsize, int ysize);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfont8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_coursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize,
    int pysize, int px0, int py0, char *buf, int bxsize);

#define COL8_000000 0
#define COL8_FF0000 1
#define COL8_00FF00 2
#define COL8_FFFF00 3
#define COL8_0000FF 4
#define COL8_FF00FF 5
#define COL8_00FFFF 6
#define COL8_FFFFFF 7
#define COL8_C6C6C6 8
#define COL8_840000 9
#define COL8_008400 10
#define COL8_848400 11
#define COL8_000084 12
#define COL8_840084 13
#define COL8_008484 14
#define COL8_848484 15

struct BOOTINFO {
    char cyls, leds, vmode, reserve;
    short scrnx, scrny;
    char *vram;
};

struct SEGMENT_DESCRIPTOR {         // Global (segment) descriptor table
    short limit_low, base_low;      // 2, 2 セグメントの大きさ
    char base_mid, access_right;    // 1, 1 どの番地から始まるか
    char limit_high, base_high;     // 1, 1 管理用属性？
};

struct GATE_DESCRIPTOR {            // interrupt descriptor table(割り込み記述子表)
    short offset_low, selector;     // 2, 2
    char dw_count, access_right;    // 1, 1
    short offset_high;              // 2
};

void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
    char s[40];
    char mcursor[256];
    int mx = 120, my =120;

    init_palette();
    init_screen(binfo -> vram, binfo -> scrnx, binfo -> scrny);

    putfont8_asc(binfo -> vram, binfo -> scrnx,  9,  9, COL8_000000, "Haribote OS.");
    putfont8_asc(binfo -> vram, binfo -> scrnx,  8,  8, COL8_FFFFFF, "Haribote OS.");

    sprintf(s, "scrnx = %d", binfo -> scrnx);
    putfont8_asc(binfo -> vram, binfo -> scrnx, 16, 64, COL8_FFFFFF, s);

    init_mouse_coursor8(mcursor, COL8_008484);
    putblock8_8(binfo -> vram, binfo -> scrnx, 16, 16, mx, my, mcursor, 16);

    for ( ; ; ) {
        io_hlt();
    }
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

void init_mouse_coursor8(char *mouse, char bc) {
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

void putfont8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s) {
    extern char hankaku[4096];     // ../hankaku.c の中にある
    for (; *s != 0x00; s++) {       // 文字列の最後(0x00)まで回す
        putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
        x += 8;
    }
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
}
