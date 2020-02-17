struct BOOTINFO {   // 0x0ff0 - 0x0fff
    char cyls;          // ブートセクタがどこまでディスクを読んだか
    char leds;          // ブート時キーボードのLED状態
    char vmode;         // ビデオモード(何ビットカラーか)
    char reserve;
    short scrnx, scrny; // 画面解像度
    char *vram;
};

#define ADR_BOOTINFO    0x00000ff0

// nasmfunc.asm
extern void io_hlt(void);
extern void io_stihlt(void);
// 割り込みフラグを0にする
extern void io_cli(void);
// 割り込みフラグを1にする
extern void io_sti(void);
extern int io_in8(int port);
extern void io_out8(int port, int data);
extern int io_load_eflags(void);
extern void io_store_eflags(int eflags);
extern void load_gdtr(int limit, int addr);
extern void load_idtr(int limit, int addr);
extern void asm_inthandler20(void);
extern void asm_inthandler21(void);
extern void asm_inthandler27(void);
extern void asm_inthandler2c(void);
extern int load_cr0(void);
extern void store_cr0(int cr0);
extern void load_tr(int tr);
// extern void taskswitch3(void);
// extern void taskswitch4(void);
extern void farjmp(int eip, int cs);

// dsctbl.c
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

#define ADR_IDT         0x0026f800
#define LIMIT_IDT       0x000007ff
#define ADR_GDT         0x00270000
#define LIMIT_GDT       0x0000ffff
#define ADR_BOTPAK      0x00280000
#define LIMIT_BOTPAK    0x0007ffff
#define AR_DATA32_RW    0x4092
#define AR_CODE32_ER    0x409a
#define AR_INTGATE32    0x008e
#define AR_TSS32        0x0089

// graphic.c
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y2);
void init_screen(unsigned char *vram, int xsize, int ysize);

void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfont8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize,
    int pysize, int px0, int py0, char *buf, int bxsize);
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);

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

// sprintf.c
void sprintf (char *str, char *fmt, ...);

/* int.c */
void init_pic(void);
void inthandler27(int *esp);

struct KEYBUF {
    unsigned char data[32];
    int next_r, next_w, len;
};

#define PORT_KEYDAT     0x0060
#define PORT_KEYDATA 0x0060

#define PIC0_ICW1 0x0020
#define PIC0_OCW2 0x0020
#define PIC0_IMR 0x0021
#define PIC0_ICW2 0x0021
#define PIC0_ICW3 0x0021
#define PIC0_ICW4 0x0021
#define PIC1_ICW1 0x00a0
#define PIC1_OCW2 0x00a0
#define PIC1_IMR 0x00a1
#define PIC1_ICW2 0x00a1
#define PIC1_ICW3 0x00a1
#define PIC1_ICW4 0x00a1

struct FIFO32 {
    unsigned int *buf;
    int p;      // next_w
    int q;      // next_r
    int size;   // 何バイトの大きさかメモ
    int free;   // バッファにのこっている空き
    int flags;
    struct TASK *task;  // もしあれば，起こしたいタスク情報を記録する
};

#define FLAGS_OVERRUN 0x0001
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);
int fifo32_put(struct FIFO32 *fifo, int data);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);

// mouse.c
#define PORT_KEYDAT             0x0060
#define PORT_KEYSTA             0x0064
#define PORT_KEYCMD             0x0064  // コマンドを受け付けるようになるとデータ下から2bit目が0になる
#define KEYSTA_SEND_NOTREADY    0x02
#define KEYCMD_WRITE_MODE       0x60    // モード設定するよ命令
#define KBC_MODE                0x47    // マウス利用するためのモード
#define KEYCMD_SENDTO_MOUSE     0xd4    // この命令の次の命令をマウスに送る
#define MOUSECMD_ENABLE         0xf4

struct MOUSE_DEC {
    unsigned char buf[3], phase;
    int x, y, btn;
};

void inthandler2c(int *esp);
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, int dat);
extern void asm_inthandler27(void);

// keyboard.c
static char keytable[0x54] = {
    0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.'
};

void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32 *fifo, int data0);
void inthandler21(int *esp);

// memory.c
#define EFLAGS_AC_BIT       0x00040000
#define CR0_CACHE_DISABLE   0x60000000
#define MEMMAN_FREES         4090    // これで約32KB
#define MEMMAN_ADDR 0x003c0000  // memmanの使うアドレス

struct FREEINFO {   // 空き情報
    unsigned int addr, size;
};

struct MEMMAN {      // メモリ管理
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start, unsigned int end);
unsigned int memtest_sub(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

// sheet.c
// 重ね合わせの下敷き
struct SHEET {
    unsigned char *buf;
    int bxsize, bysize; // 全体のサイズ
    int vx0, vy0;   // 画面上の座標
    int col_inv;    // 透明色番号
    int height; // 高さ
    int flags;
    struct SHTCTL *ctl;
};

#define MAX_SHEETS  256

// SHEETを管理する
struct SHTCTL {
    unsigned char *vram, *map;    // VRAMのアドレスを保存
    int xsize, ysize;   // 画面サイズを記録
    int top;    // 下敷き一番上の高さ
    struct SHEET *sheets[MAX_SHEETS];   // 番地変数を記録(順番をころころさせるためにアドレスで...)
    struct SHEET sheets0[MAX_SHEETS];   // 下敷き情報256枚分実体
};

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);
void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0);

// timer.c
#define PIT_CTRL    0x0043  // IRQ0割り込み周期変更につかうBIOSファンクション指定
#define PIT_CNT0    0x0040  // 割り込み周期変更内容を書き込むところ
#define MAX_TIMER   500

struct TIMER {
    struct TIMER *next;
    unsigned int timeout, flags;
    struct FIFO32 *fifo;
    int data;
};

struct TIMERCTL {
    unsigned int count, next;
    // unsigned int using;    // 使っている個数を記録していた...過去のもの
    struct TIMER *t0;
    struct TIMER timers0[MAX_TIMER];
} timerctl;

void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_settime(struct TIMER *timer, unsigned int timeout);
void inthandler20(int *esp);
// void settimer(unsigned int timeout, struct FIFO8 *fifo, unsigned char data);

// mtask.c
#define MAX_TASKS   1000    // 最大タスク数
#define TASK_GDT0   3       // TSSをGDTの何番から借りるか

// 途中まで実行したタスクのレジスタ情報とか戻り先アドレスを記録しておく
struct TSS32 {  // 全部で104byte
    // タスクの設定とか
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
    // 32bitレジスタ達
    //  EIP: 次の命令がどこにあるかをCPUが記録するためのレジスタ
    int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    // 16bitレジスタ
    int es, cs, ss, ds, fs, gs;
    int ldtr, icmap;
};

struct TASK {
    int sel, flags; // sel: GDT番号のこと
    int priority;   // 優先度
    struct TSS32 tss;
};

struct TASKCTL {
    int running;    // 稼働しているタスク数
    int now;        // 現在動作しているタスクがわかるようにする
    struct TASK *tasks[MAX_TASKS];
    struct TASK tasks0[MAX_TASKS];
};

struct TIMER *mt_timer;
struct TASKCTL *taskctl;
struct TIMER *task_timer;

struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);
void mt_init(void);
void mt_taskswitch(void);
