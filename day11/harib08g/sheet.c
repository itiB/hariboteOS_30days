#include "bootpack.h"

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize) {
    struct SHTCTL *ctl;
    int i;

    // 下敷き制御変数を記録するためのメモリ確保
    ctl = (struct SHTCTL *) memman_alloc_4k(memman, sizeof(struct SHTCTL));
    if (ctl == 0) {
        goto err;
    }
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1;    // シートが一枚もない時
    for(i = 0; i < MAX_SHEETS; i++) {
        ctl->sheets0[i].flags = 0;  // 未使用マーク
        ctl->sheets0[i].ctl = ctl;  // 所属を記録
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
        if (ctl->sheets0[i].flags == 0) {
            sht = &ctl->sheets0[i];
            sht->flags = SHEET_USE;   // 使用中マーク
            sht->height = -1; // 非表示中(まだ設定していないから)
            return sht;
        }
    }
    return 0;   // すべてのシートが使用中だった
}

// 下敷きのバッファや大きさ，透明色を設定する関数
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv) {
    sht->buf = buf;
    sht->bxsize = xsize;
    sht->bysize = ysize;
    sht->col_inv = col_inv;
    return;
};

// 下敷きの上下を並べ替える
void sheet_updown(struct SHEET *sht, int height) {
    int h, old = sht->height;  // 設定前の高さを記録
    struct SHTCTL *ctl = sht->ctl;

    // 指定が高すぎたり低すぎたりしたら変更
    if (height > ctl->top + 1) {
        height = ctl->top + 1;
    }
    if (height < -1) {
        height = -1;
    }
    sht->height = height; // 高さを設定

    // 以下は主にsheets[]の並び替え
    if (old > height) { // 以前のものより後ろに移動するとき
        if (height >= 0) {
            // 間にあるものを前に出す
            for (h = old; h > height; h--) {
                ctl->sheets[h] = ctl->sheets[h - 1];
                ctl->sheets[h]->height= h;
            }
            ctl->sheets[height] = sht;
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
        } else {    // 非表示化
            if (ctl->top > old) {
                // 上になっているものをおろす
                for (h = old; h < ctl->top; h++) {
                    ctl->sheets[h] = ctl->sheets[h + 1];
                    ctl->sheets[h]->height= h;
                }
            }
            ctl->top --;  // 表示中の下敷きが一つ無くなるので一番上の高さが減る
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
        }
        // sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize); // 新しい下敷きの情報にそって画面を書き出す
    } else if (old < height) {  // 以前よりも前に画面を表示する
        if (old >= 0) {
            // 間のものを押し下げる
            for (h = old; h < height; h++) {
                ctl->sheets[h] = ctl->sheets[h + 1];
                ctl->sheets[h]->height= h;
            }
            ctl->sheets[height] = sht;
        } else {    // 非表示状態から表示させる
            // 上に来るものを持ち上げる
            for (h = ctl->top; h >= height; h--) {
                ctl->sheets[h + 1] = ctl->sheets[h];
                ctl->sheets[h + 1]->height= h;
            }
            ctl->sheets[height] = sht;
            ctl->top++;   // 表示中の下敷きが一つ増えるから高さが増える
        }
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height); // 新しい下敷きの情報にそって画面を書き出す
    }
    return;
}

void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1) {
    if (sht->height >= 0) {
        sheet_refreshsub(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height);
    }
    return;
}

// 画面の書き換わる範囲だけを書き換える(vx0 左辺，vx1 右辺)
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0) {
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char *buf, c, *vram = ctl->vram;
    struct SHEET *sht;

    // refreshが画面外にはみ出していたら修正
    if (vx0 < 0) { vx0 = 0; }
    if (vy0 < 0) { vy0 = 0; }
    if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
    if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }

    for (h = h0; h <= ctl->top; h++) {
        sht = ctl->sheets[h];
        buf = sht ->buf;
        // vx0 - vy1をつかってbx0 - by1を逆算する
        bx0 = vx0 - sht->vx0;   // 書き換える範囲をbx0 - bx1にする
        by0 = vy0 - sht->vy0;
        bx1 = vx1 - sht->vx0;
        by1 = vy1 - sht->vy0;
        if (bx0 < 0) { bx0 = 0; }   // 少しだけ重なった時用の処理
        if (by0 < 0) { by0 = 0; }   //   書き換え範囲は上に重なった下敷きの角
        if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }   // 同じく少しだけ重なったとき
        if (by1 > sht->bysize) { by1 = sht->bysize; }   //   右下が求められない時用
        for (by = by0; by < by1; by++) {
            vy = sht->vy0 + by;
            for (bx = bx0; bx < bx1; bx++) {
                vx = sht->vx0 + bx;
                c = buf[by * sht->bxsize + bx];
                if (c != sht->col_inv) {
                    vram[vy * ctl->xsize + vx] = c;
                }
            }
        }
    }
}

// 下敷きの高さは変えずに上下左右に動かす
void sheet_slide(struct SHEET *sht, int vx0, int vy0) {
    int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if (sht->height >= 0) {
        // もしも表示中ならば 新しい下敷き情報にそって画面を書き出す
        sheet_refreshsub(sht->ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
        sheet_refreshsub(sht->ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);
    }
    return;
}

// 使い終わった下敷きを解放する
void sheet_free(struct SHEET *sht) {
    if (sht->height >= 0) {
        sheet_updown(sht, -1); // 表示中ならまず非表示にする
    }
    sht->flags = 0;   // 未使用マーク
    return;
}