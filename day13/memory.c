#include "bootpack.h"

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
        cr0 |= CR0_CACHE_DISABLE;   // キャッシュを許可しない
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

// memmanを初期化して空にする
void memman_init(struct MEMMAN *man) {
    man->frees = 0;       // 空き情報の個数
    man->maxfrees = 0;    // 状況観察用: freesの最大値
    man->lostsize = 0;    // 解放に失敗した合計サイズ
    man->losts = 0;      // 解放に失敗した回数
    return;
}

// 空きサイズ合計を報告
unsigned int memman_total(struct MEMMAN *man) {
    unsigned int i, total = 0;
    for (i = 0; i < man->frees; i++) {
        total += man->free[i].size;
    }
    return total;
}

// 確保する関数
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size) {
    unsigned int i, a;
    for (i = 0; i < man->frees; i++) {
        if (man->free[i].size >= size) {
            // 十分な空きを見つける
            a = man->free[i].addr;
            man->free[i].addr += size;
            man->free[i].size -= size;
            if (man->free[i].size == 0) {
                // free[i]が無くなったので前へつめる
                man->frees--;     // 空き情報の数を減らす
                for (; i < man->frees; i++) {
                    man->free[i] = man->free[i + 1];    // 構造体の代入
                }
            }
            return a;
        }
    }
    return 0;   // 空きがない
}

// 空きメモリの解放(memmanに追加する)
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size) {
    int i, j;
    // まとめるためにfree[]がaddr順に並べたい
    // どこに入れるべきか考える
    for (i = 0; i < man->frees; i++) {
        if (man->free[i].addr > addr) {
            break;
        }
    }
    // free[i - a].addr < addr < free[i].addr
    if (i > 0) {
        // 前があるとき
        if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
            // 前の空き領域とまとめて
            man->free[i - 1].size += size;
            if (i < man->frees) {
                // 後ろがあるとき
                if (addr + size == man->free[i].addr) {
                    // 後ろもまとめる
                    man->free[i - 1].size += man->free[i].size;
                    // man->free[i]を削除する
                    man->frees--;
                    for (; i < man->frees; i++) {
                        man->free[i] = man->free[i + 1];  // 構造体の代入
                    }
                }
            }
            return 0;
        }
    }
    // 前とはまとめられなかったとき
    if (i < man->frees) {
        // 後ろがあるとき
        if (addr + size == man->free[i].addr) {
            // 後ろとはまとめる
            man->free[i].addr = addr;
            man->free[i].size += size;
            return 0;   // 成功終了
        }
    }
    // 前にもうしろにもまとめられず気合で差し込む
    if (man->frees < MEMMAN_FREES) {
        // free[i]よりも後ろをひとつ後ろにずらして隙間を作る
        for (j = man->frees; j > i; j--) {
            man->free[j] = man->free[j - 1];
        }
        man->frees ++;
        if (man->maxfrees < man->frees) {
            man->maxfrees = man->frees; // 最大値の更新
        }
        man->free[i].addr = addr;
        man->free[i].size = size;
        return 0;
    }
    // 後ろにずらせなかった
    man->losts++;
    man->lostsize += size;
    return -1;
}

// 細かくメモリ確保すると面倒だから0x1000byte単位でalloc
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size) {
    unsigned int a;
    size = (size + 0xfff) & 0xfffff000; // 0xfffff000 - 0x00000001した値をsizeに足してから切り捨て
    a = memman_alloc(man, size);
    return a;
}

// 細かくメモリ確保すると面倒だから0x1000byte単位でfree
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size) {
    int i;
    size = (size + 0xfff) & 0xfffff000;
    i = memman_free(man, addr, size);
    return i;
}
