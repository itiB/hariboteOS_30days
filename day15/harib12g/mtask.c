#include "bootpack.h"

int mt_tr;

// mt_trとmt_timerを初期化してタイマを0.02秒後にセット
void mt_init(void) {
    mt_timer = timer_alloc();
    // timer_initは必要ないからしない(FIFOバッファに書かなくていいらしい)
    timer_settime(mt_timer, 2);
    mt_tr = 3 * 8;
    return;
}

// mt_trをもとに次のmt_trを計算してタイマをセットし直す．
// その後タスクswitch
void mt_taskswitch(void) {
    if (mt_tr == 3 * 8) {
        mt_tr = 4 * 8;
    } else {
        mt_tr = 3 * 8;
    }
    timer_settime(mt_timer, 2);
    farjmp(0, mt_tr);
    return;
}