#include "bootpack.h"

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize) {
    struct SHTCTL *ctl;
    int i;

    // 下敷き制御変数を記録するためのメモリ確保
    ctl = (struct SHTCTL *) memman_alloc_4k(memman, sizeof(struct SHTCTL));
    if (ctl == 0) {
        goto err;
    }

    // Mapを追加する
    ctl->map = (unsigned char *) memman_alloc_4k(memman, xsize * ysize);
    if (ctl->map == 0) {
        memman_free_4k(memman, (int) ctl, sizeof (struct SHTCTL));
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
            sht->flags  = SHEET_USE;   // 使用中マーク
            sht->height = -1; // 非表示中(まだ設定していないから)
            sht->task = 0;  // 自動で閉じる機能を通常は使わない(task割り当てのときのみ使う)
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
}

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
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
            sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, old);
        } else {    // 非表示化
            if (ctl->top > old) {
                // 上になっているものをおろす
                for (h = old; h < ctl->top; h++) {
                    ctl->sheets[h] = ctl->sheets[h + 1];
                    ctl->sheets[h]->height = h;
                }
            }
            ctl->top--;  // 表示中の下敷きが一つ無くなるので一番上の高さが減る
            sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, old - 1);
        }
        // sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize); // 新しい下敷きの情報にそって画面を書き出す
    } else if (old < height) {  // 以前よりも前に画面を表示する
        if (old >= 0) {
            // 間のものを押し下げる
            for (h = old; h < height; h++) {
                ctl->sheets[h] = ctl->sheets[h + 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        } else {    // 非表示状態から表示させる
            // 上に来るものを持ち上げる
            for (h = ctl->top; h >= height; h--) {
                ctl->sheets[h + 1] = ctl->sheets[h];
                ctl->sheets[h + 1]->height = h + 1;
            }
            ctl->sheets[height] = sht;
            ctl->top++;   // 表示中の下敷きが一つ増えるから高さが増える
        }
        sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height); // 新しい下敷きの情報にそって画面を書き出す
    }
    return;
}

void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1) {
    if (sht->height >= 0) {
        sheet_refreshsub(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height, sht->height);
    }
    return;
}

// 画面の書き換わる範囲だけを書き換える(vx0 左辺，vx1 右辺)
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1) {
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char *buf, *vram = ctl->vram, *map = ctl->map, sid;
    struct SHEET *sht;

    // refreshが画面外にはみ出していたら修正
    if (vx0 < 0) { vx0 = 0; }
    if (vy0 < 0) { vy0 = 0; }
    if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
    if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }

    for (h = h0; h <= h1; h++) {
        sht = ctl->sheets[h];
        buf = sht->buf;
        sid = sht - ctl->sheets0;
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
                if (map[vy * ctl->xsize + vx] == sid) {
                    vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
                }
            }
        }
    }
    return;
}

// 下敷きの高さは変えずに上下左右に動かす
void sheet_slide(struct SHEET *sht, int vx0, int vy0) {
    struct SHTCTL *ctl = sht->ctl;
    int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if (sht->height >= 0) {
        // 移動元と移動先のMapを作り直す
        sheet_refreshmap(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
        sheet_refreshmap(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);
        // もしも表示中ならば 新しい下敷き情報にそって画面を書き出す
        // 移動元 下にいた下敷きだけを書き直す
        sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height - 1);
        // 移動先 移動した一枚だけを書き直す
        sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height);
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

// 下敷きMapにどこのWindowが一番上にあるかを書き込む関数
void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0) {
    int h, bx, by, vx, vy, bx0, by0, bx1, by1, sid4, *p;
    unsigned char *buf, sid, *map = ctl->map;
    struct SHEET *sht;
    if (vx0 < 0) { vx0 = 0; }
    if (vy0 < 0) { vy0 = 0; }
    if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
    if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }
    for (h = h0; h <= ctl->top; h++) {
        sht = ctl->sheets[h];
        sid = sht - ctl->sheets0;   // 番地を引き算して下敷き番号として使う
        buf = sht->buf;
        bx0 = vx0 - sht->vx0;
        by0 = vy0 - sht->vy0;
        bx1 = vx1 - sht->vx0;
        by1 = vy1 - sht->vy0;
        if (bx0 < 0) { bx0 = 0; }
        if (by0 < 0) { by0 = 0; }
        if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
        if (by1 > sht->bysize) { by1 = sht->bysize; }
        if (sht->col_inv == -1) {
            // 透明色なしなのですべて書き換える
            if ((sht->vx0 & 3) == 0 && (bx0 & 3) == 0 && (bx1 & 3) == 0) {
                // 透明色がない時，一気に4byte書き込む(32bitレジスタを使う)
                bx1 = (bx1 - bx0) / 4; // MOV命令の回数
                sid4 = sid | sid << 8 | sid << 16 | sid << 24;
                for (by = by0; by < by1; by++) {
                    vy = sht->vy0 + by;
                    vx = sht->vx0 + bx0;
                    p = (int *) &map[vy * ctl->xsize + vx];
                    for (bx = 0; bx < bx1; bx++) {
                        p[bx] = sid4;
                    }
                }
            } else {
                // 一気に書き込めないとき用
                for (by = by0; by < by1; by++) {
                    vy = sht->vy0 + by;
                    for (bx = bx0; bx < bx1; bx++) {
                        vx = sht->vx0 + bx;
                        map[vy * ctl->xsize + vx] = sid;
                    }
                }
            }
        } else {
            // マウスとか透明色あるので判別して書き換え
            for (by = by0; by < by1; by++) {
                vy = sht->vy0 + by;
                for (bx = bx0; bx < bx1; bx++) {
                    vx = sht->vx0 + bx;
                    // 下敷きの透明部分か不透明部分かを判別
                    if (buf[by * sht->bxsize + bx] != sht->col_inv) {
                        // メモリのどこかに1byte書き込む
                        map[vy * ctl->xsize + vx] = sid;
                    }
                }
            }
        }
    }
    return;
}