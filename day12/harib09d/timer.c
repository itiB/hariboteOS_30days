#include "bootpack.h"

#define TIMER_FLAGS_ALLOC   1   // 確保した状態
#define TIMER_FLAGS_USING   2   // タイマ動作中

void init_pit(void) {
    int i;
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c); // 割り込み周期 11932回（１秒間に１００回割り込み)(0x2e9c)
    io_out8(PIT_CNT0, 0x2e);
    timerctl.count = 0;
    for (i = 0; i < MAX_TIMER; i++) {
        timerctl.timer[i].flags = 0;    // 未使用
    }
    return;
}

struct TIMER *timer_alloc(void) {
    int i;
    for (i = 0; i < MAX_TIMER; i++) {
        if (timerctl.timer[i].flags == 0) {
            timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timer[i];
        }
    }
    return 0;   // 空いているタイマーが見つからなかった
}

void timer_free(struct TIMER *timer) {
    timer->flags = 0;   // 未使用
    return;
}

void timer_init(struct TIMER *timer, struct FIFO8 *fifo, unsigned char data) {
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout) {
    timer->timeout = timeout;
    timer->flags = TIMER_FLAGS_USING;
    return;
}

// タイマ割り込み(IRQ0)が発生したときに呼び出される割り込みハンドラ
// もしタイムアウトが設定されているならば呼び出されるたびにタイマーを減らしていく
void inthandler20(int *esp) {
    int i;
    io_out8(PIC0_OCW2, 0x60);   // IRQ-00受付完了をPICに通知
    timerctl.count++;   // 1秒に100ずつ増えていくカウンタ(割り込み発生のたびに増える)
    for (i = 0; i < MAX_TIMER; i++) {
        if (timerctl.timer[i].flags == TIMER_FLAGS_USING) {
            timerctl.timer[i].timeout--;
            if (timerctl.timer[i].timeout == 0) {
                timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
                fifo8_put(timerctl.timer[i].fifo, timerctl.timer[i].data);
            }
        }
    }
    return;
}

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