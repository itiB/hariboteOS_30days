#include "bootpack.h"

#define TIMER_FLAGS_ALLOC   1   // 確保した状態
#define TIMER_FLAGS_USING   2   // タイマ動作中

void init_pit(void) {
    int i;
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c); // 割り込み周期 11932回（１秒間に１００回割り込み)(0x2e9c)
    io_out8(PIT_CNT0, 0x2e);
    timerctl.count = 0;
    timerctl.next  = 0xffffffff;    // 最初は作動中のタイマがないので初期値
    timerctl.using = 0;
    for (i = 0; i < MAX_TIMER; i++) {
        timerctl.timers0[i].flags = 0;    // 未使用
    }
    return;
}

struct TIMER *timer_alloc(void) {
    int i;
    for (i = 0; i < MAX_TIMER; i++) {
        if (timerctl.timers0[i].flags == 0) {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timers0[i];
        }
    }
    return 0;   // 空いているタイマーが見つからなかった
}

void timer_free(struct TIMER *timer) {
    timer->flags = 0;   // 未使用
    return;
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data) {
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout) {
    int e, i, j;
    timer->timeout = timeout + timerctl.count;  // 今のカウント数と鳴らすまでを足してタイムアウトする予定時刻を入れる
    timer->flags = TIMER_FLAGS_USING;
    e = io_load_eflags();   // 割り込みを禁止しないと行けない
    io_cli();
    for (i = 0; i < timerctl.using; i++) {
        if (timerctl.timers[i]->timeout >= timer->timeout) {
            break;
        }
    }
    // 後ろをずらす
    for (j = timerctl.using; j > i; j--) {
        timerctl.timers[j] = timerctl.timers[j - 1];
    }
    timerctl.using++;
    // 空いた隙間に入れていく
    timerctl.timers[i] = timer;
    timerctl.next = timerctl.timers[0]->timeout;
    io_store_eflags(e); // 割り込みを許可
    return;
}

// タイマ割り込み(IRQ0)が発生したときに呼び出される割り込みハンドラ
// もしタイムアウトが設定されているならば呼び出されるたびにタイマーを減らしていく
void inthandler20(int *esp) {
    int i, j;
    io_out8(PIC0_OCW2, 0x60);   // IRQ-00受付完了をPICに通知
    timerctl.count++;   // 1秒に100ずつ増えていくカウンタ(割り込み発生のたびに増える)

    if (timerctl.next > timerctl.count) {
        return; // まだ次の時刻になっていないから気にしない．
    }
    timerctl.next = 0xffffffff;

    for (i = 0; i < timerctl.using; i++) {
        // timersのなかのタイマはすべて稼働中のもの
        if (timerctl.timers[i]->timeout > timerctl.count) {  // timeout -> 残り時間から予定時刻に
            break;
        }
        // タイムアウトしたタイマの処理
        timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
        fifo32_put(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
    }
    timerctl.using -= i;    // i個目のタイマがタイムアウトしたからずらす
    for (j = 0; j < timerctl.using; j++) {
        timerctl.timers[j] = timerctl.timers[i + j];
    }
    if (timerctl.using > 0) {
        timerctl.next = timerctl.timers[0]->timeout;
    } else {
        timerctl.next = 0xffffffff;
    }
    return;
}


// 時刻調整用プログラム...
// void timer_reset() {
//     int t0 = timerctl.count;    // すべての時刻から引く値
//     io_cli();   // 割り込まれると困るので...
//     timerctl.count -= t0;
//     for (i = 0; i < MAC_TIMER; i++) {
//         if (timerctl.timer[i].flags == TIMER_FLAGS_USING) {
//             timerctl.timer[i].timeout -= t0;
//         }
//     }
//     io_sti();
// }

// void settimer(unsigned int timeout, struct FIFO8 *fifo, unsigned char data) {
//     int eflags;
//     eflags = io_load_eflags();  // 割り込みフラグをロード
//     io_cli();
//     timerctl.timeout = timeout;
//     timerctl.fifo = fifo;
//     timerctl.data = data;
//     io_store_eflags(eflags);    // 割り込みを許可する
//     return;
// }