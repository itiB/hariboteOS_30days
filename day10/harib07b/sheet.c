#include "bootpack.h"

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize) {
    struct SHTCTL *ctl;
    int i;

    // 下敷き制御変数を記録するためのメモリ確保
    ctl = (struct SHTCTL *) memman_alloc_4k(memman, sizeof(struct SHTCTL));
    if (ctl == 0) {
        goto err;
    }
    ctl -> vram = vram;
    ctl -> xsize = xsize;
    ctl -> ysize = ysize;
    ctl -> top = -1;    // シートが一枚もない時
    for(i = 0; i < MAX_SHEETS; i++) {
        ctl -> sheets0[i].flags = 0;    // 未使用マーク
    }
err:
    return ctl;
}

#define SHEET_USE   1

// 新規に未使用の下敷きをもらってくる関数
struct SHEET *sheet_alloc(struct SHTCTL *ctl) {
    struct SHEET *sht;
    int i;
    for (i = 0; i < MAX_SHEETS; i++) {
        if (ctl -> sheets0[i].flags == 0) {
            sht = &ctl -> sheets0[i];
            sht -> flags = SHEET_USE;   // 使用中マーク
            sht -> height = -1; // 非表示中(まだ設定していないから)
            return sht;
        }
    }
    return 0;   // すべてのシートが使用中だった
}

// 下敷きのバッファや大きさ，透明色を設定する関数
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv) {
    sht -> buf = buf;
    sht -> bxsize = xsize;
    sht -> bysize = ysize;
    sht -> col_inv = col_inv;
    return;
};

void sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height) {
    int h, old = sht -> height;  // 設定前の高さを記録

    // 指定が高すぎたり低すぎたりしたら変更
    if (height > ctl -> top + 1) {
        height = ctl -> top + 1;
    }
    if (height < -1) {
        height = -1;
    }
    sht -> height = height; // 高さを設定

    // 以下は主にsheets[]の並び替え
    if (old > height) { // 以前のものより後ろに移動するとき
        if (height >= 0) {
            // 間にあるものを前に出す
            for (h = old; h > height; h--) {
                ctl -> sheets[h] = ctl -> sheets[h - 1];
                ctl -> sheets[h] -> height= h;
            }
            ctl -> sheets[height] = sht;
        } else {    // 非表示化
            if (ctl -> top > old) {
                // 上になっているものをおろす
                for (h = old; h < ctl -> top; h++) {
                    ctl -> sheets[h] = ctl -> sheets[h + 1];
                    ctl -> sheets[h] -> height= h;
                }
            }
            ctl -> top --;  // 表示中の下敷きが一つ無くなるので一番上の高さが減る
        }
        sheet_refresh(ctl); // 新しい下敷きの情報にそって画面を書き出す
    } else if (old < height) {  // 以前よりも前に画面を表示する
        if (old >= 0) {
            // 間のものを押し下げる
            for (h = old; h < height; h++) {
                ctl -> sheets[h] = ctl -> sheets[h + 1];
                ctl -> sheets[h] -> height= h;
            }
            ctl -> sheets[height] = sht;
        } else {    // 非表示状態から表示させる
            // 上に来るものを持ち上げる
            for (h = ctl -> top; h >= height; h--) {
                ctl -> sheets[h + 1] = ctl -> sheets[h];
                ctl -> sheets[h + 1] -> height= h;
            }
            ctl -> sheets[height] = sht;
            ctl -> top++;   // 表示中の下敷きが一つ増えるから高さが増える
        }
        sheet_refresh(ctl); // 新しい下敷きの情報にそって画面を書き出す
    }
    return;
}

void sheet_refresh(struct SHTCTL *ctl) {
    int h, bx, by, vx, vy;
    unsigned char *buf, c, *vram = ctl -> vram;
    struct SHEET *sht;
    for (h = 0; h <= ctl -> top; h++) {
        sht = ctl -> sheets[h];
        buf = sht ->buf;
        for (by = 0; by < sht -> bysize; by++) {
            vy = sht -> vy0 + by;
            for (bx = 0; bx < sht -> bxsize; bx++) {
                vx = sht -> vx0 + bx;
                c = buf[by * sht -> bxsize + bx];
                if (c != sht -> col_inv) {
                    vram[vy * ctl -> xsize + vx] = c;
                }
            }
        }
    }
}

// 下敷きの高さは変えずに上下左右に動かす
void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int vx0, int vy0) {
    sht -> vx0 = vx0;
    sht -> vy0 = vy0;
    if (sht -> height >= 0) {
        // もしも表示中ならば
        sheet_refresh(ctl); // 新しい下敷き情報にそって画面を書き出す
    }
    return;
}

// 使い終わった下敷きを解放する
void sheet_free(struct SHTCTL *ctl, struct SHEET *sht) {
    if (sht -> height >= 0) {
        sheet_updown(ctl, sht, -1); // 表示中ならまず非表示にする
    }
    sht -> flags = 0;   // 未使用マーク
    return;
}