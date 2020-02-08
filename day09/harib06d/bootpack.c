#include "bootpack.h"

#define EFLAGS_AC_BIT       0x00040000
#define CR0_CACHE_DISABLE   0x60000000

unsigned int memtest(unsigned int start, unsigned int end);
unsigned int memtest_sub(unsigned int start, unsigned int end);

#define MEMMAN_FREES         4090    // これで約32KB

struct FREEINFO {   // 空き情報
    unsigned int addr, size;
};

struct MEMMAN {      // メモリ管理
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMAN_FREES];
};

void memman_init(struct MEMMAN *man) {
    // memmanを初期化して空にする
    man -> frees = 0;       // 空き情報の個数
    man -> maxfrees = 0;    // 状況観察用: freesの最大値
    man -> lostsize = 0;    // 解放に失敗した合計サイズ
    man -> losts = 0;      // 解放に失敗した回数
    return;
}

unsigned int memman_total(struct MEMMAN *man) {
    // 空きサイズ合計を報告
    unsigned int i, total = 0;
    for (i = 0; i < man -> frees; i++) {
        total += man -> free[i].size;
    }
    return total;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size) {
    // 確保する関数
    unsigned int i, a;
    for (i = 0; i < man -> frees; i++) {
        if (man -> free[i].size >= size) {
            // 十分な空きを見つける
            a = man -> free[i].addr;
            man -> free[i].addr += size;
            man -> free[i].size -= size;
            if (man -> free[i].size == 0) {
                // free[i]が無くなったので前へつめる
                man -> frees--;     // 空き情報の数を減らす
                for (; i < man -> frees; i++) {
                    man -> free[i] = man -> free[i + 1];    // 構造体の代入
                }
            }
            return a;
        }
    }
    return 0;   // 空きがない
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size) {
    // 空きメモリの解放(memmanに追加する)
    int i, j;
    // まとめるためにfree[]がaddr順に並べたい
    // どこに入れるべきか考える
    for (i = 0; i < man -> frees; i++) {
        if (man -> free[i].addr > addr) {
            break;
        }
    }
    // free[i - a].addr < addr < free[i].addr
    if (i > 0) {
        // 前があるとき
        if (man -> free[i - 1].addr + man -> free[i - 1].size == addr) {
            // 前の空き領域とまとめて
            man -> free[i - 1].size += size;
            if (i < man -> frees) {
                // 後ろがあるとき
                if (addr + size == man -> free[i].addr) {
                    // 後ろもまとめる
                    man -> free[i - 1].size += man -> free[i].size;
                    // man -> free[i]を削除する
                    man -> frees--;
                    for (; i < man -> frees; i++) {
                        man -> free[i] = man -> free[i + 1];  // 構造体の代入
                    }
                }
            }
            return 0;
        }
    }
    // 前とはまとめられなかったとき
    if (i < man -> frees) {
        // 後ろがあるとき
        if (addr + size == man -> free[i].addr) {
            // 後ろとはまとめる
            man -> free[i].addr += addr;
            man -> free[i].size -= size;
            return 0;   // 成功終了
        }
    }
    // 前にもうしろにもまとめられず気合で差し込む
    if (man -> frees < MEMMAN_FREES) {
        // free[i]よりも後ろをひとつ後ろにずらして隙間を作る
        for (j = man -> frees; j > i; j--) {
            man -> free[j] = man -> free[j - 1];
        }
        man -> frees ++;
        if (man -> maxfrees < man -> frees) {
            man -> maxfrees = man -> frees; // 最大値の更新
        }
        man -> free[i].addr = addr;
        man -> free[i].size = size;
        return 0;
    }
    // 後ろにずらせなかった
    man -> losts++;
    man -> lostsize += size;
    return -1;
}

#define MEMMAN_ADDR 0x003c0000  // memmanの使うアドレス

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
    struct MOUSE_DEC mdec;
    char s[40];
    char mcursor[256];
    char keybuf[32];
    char mousebuf[128];
    int mx = 120, my =120, i, j;
    unsigned int memtotal;
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

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

    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);    // 0x00001000 - 0x0009efff
    memman_free(memman, 0x00400000, memtotal - 0x00400000);
    sprintf(s, "memory %d MB    free : %d KB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
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
