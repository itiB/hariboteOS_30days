struct BOOTINFO {   // 0x0ff0 - 0x0fff
    char cyls;          // ブートセクタがどこまでディスクを読んだか
    char leds;          // ブート時キーボードのLED状態
    char vmode;         // ビデオモード(何ビットカラーか)
    char reserve;
    short scrnx, scrny; // 画面解像度
    char *vram;
};
#define ADR_BOOTINFO    0x00000ff0

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

// nasmfunc.asm
void io_hlt(void);
void io_cli(void);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);

// dsctbl.c
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);

// garphic.c
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y2);
void init_screen(unsigned char *vram, int xsize, int ysize);

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

// bootpack.c
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfont8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_coursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize,
    int pysize, int px0, int py0, char *buf, int bxsize);

// sprintf.c
void sprintf (char *str, char *fmt, ...);