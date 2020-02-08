#include "bootpack.h"

#define EFLAGS_AC_BIT       0x00040000
#define CR0_CACHE_DISABLE   0x60000000

unsigned int memtest(unsigned int start, unsigned int end);
unsigned int memtest_sub(unsigned int start, unsigned int end);

// メモリの容量をチェックする
unsigned int memtest(unsigned int start, unsigned int end) {
    char flg486 = 0;
    unsigned int eflg, cr0, i;

    // 386なのか，486以降なのか確認
    eflg = io_load_eflags();
    eflg |= EFLAGS_AC_BIT;          // AC-Bit = 1
    io_store_eflags(eflg);          // 386に存在しないレジスタ(18番)に1を書き込む
    eflg = io_load_eflags();
    if ((eflg & EFLAGS_AC_BIT) != 0) {
        flg486 = 1;                 // レジスタが1になっているならば486以降
    }
    eflg &= ~EFLAGS_AC_BIT;         // AC-BIt = 0
    io_store_eflags(eflg);          // もとにもどして書き込み

    if (flg486 != 0) {
        cr0 = load_cr0();           // フラグを操作するために読み出す
        cr0 |= CR0_CACHE_DISABLE;  // キャッシュを許可しない
        store_cr0(cr0);
    }

    i = memtest_sub(start, end);

    if (flg486 != 0) {
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE;  // キャッシュ許可
        store_cr0(cr0);
    }
    return i;
}

unsigned int memtest_sub(unsigned int start, unsigned int end) {
    unsigned int i, *p, old;
    unsigned int pat0 = 0xaa55aa55;
    unsigned int pat1 = 0x55aa55aa;

    for (i = start; i <= end; i += 0x1000) {
        p = (unsigned int *) (i + 0xfcc);     // ポインタに書き換えないとメモリを読み書きできないから
        old = *p;           // いじる前の値を覚えておく
        *p = pat0;          // 試し書き
        *p ^= 0xffffffff;   // 反転させる
        if (*p != pat1) {   // 反転させた結果があっているか
not_memory:
            *p = old;
            break;
        }
        *p ^= 0xffffffff;   // もう一度反転させる
        if (*p != pat0) {
            goto not_memory;
        }
        *p = old;           // いじった値をもとに戻す
    }
    return i;
}


void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
    struct MOUSE_DEC mdec;
    char s[40];
    char mcursor[256];
    char keybuf[32];
    char mousebuf[128];
    int mx = 120, my =120, i, j;

    init_gdtidt();
    init_pic();
    io_sti();   // IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除
    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);
    io_out8(PIC0_IMR, 0xf9); /* pic1とキーボード許可 */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可 */

    init_keyboard();

    init_palette();
    init_screen(binfo -> vram, binfo -> scrnx, binfo -> scrny);

    enable_mouse(&mdec);
    init_mouse_coursor8(mcursor, COL8_008484);

    i = memtest(0x00400000, 0xbfffffff) / (1024 * 1024);
    sprintf(s, "memory %d MB", i);
    putfont8_asc(binfo -> vram, binfo -> scrnx, 0, 32, COL8_00FFFF, s);

    for ( ; ; ) {
        io_cli();

        if ((fifo8_status(&keyfifo) + fifo8_status(&mousefifo)) == 0) {
            io_stihlt();
        } else {
            if (fifo8_status(&keyfifo) != 0) {
                i = fifo8_get(&keyfifo);
                io_sti();
                sprintf(s, "%x", i);
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
                putfont8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
            } else if (fifo8_status(&mousefifo) != 0) {
                i = fifo8_get(&mousefifo);
                io_sti();
                if (mouse_decode(&mdec, i) != 0) {
                    // マウスデータ3バイトがそろったので表示
                    sprintf(s, "[lcr %d %d]", mdec.x, mdec.y);
                    if ((mdec.btn & 0x01) != 0) {
                        s[1] = 'L';
                    }
                    if ((mdec.btn & 0x02) != 0) {
                        s[3] = 'R';
                    }
                    if ((mdec.btn & 0x04) != 0) {
                        s[2] = 'C';
                    }
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfont8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);

                    // マウスカーソルの移動
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);     // マウスを消す
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo -> scrnx - 16) {
                        mx = binfo -> scrnx - 16;
                    }
                    if (my > binfo -> scrny - 16) {
                        my = binfo -> scrny - 16;
                    }
                    sprintf(s, "(%d, %d)", mx, my);
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15); // 座標を消す
                    putfont8_asc(binfo->vram, binfo->scrnx,  0, 0, COL8_FFFFFF, s); // 座標を書く
                    putblock8_8(binfo -> vram, binfo -> scrnx, 16, 16, mx, my, mcursor, 16);    // マウスを描く
                }
            }
        }
    }
}
