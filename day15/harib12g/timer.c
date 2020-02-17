#include "bootpack.h"

#define TIMER_FLAGS_ALLOC   1   // 確保した状態
#define TIMER_FLAGS_USING   2   // タイマ動作中

void init_pit(void) {
    int i;
    struct TIMER *t;
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c); // 割り込み周期 11932回（１秒間に１００回割り込み)(0x2e9c)
    io_out8(PIT_CNT0, 0x2e);
    timerctl.count = 0;
    for (i = 0; i < MAX_TIMER; i++) {
        timerctl.timers0[i].flags = 0;    // 未使用
    }
    t = timer_alloc();  // 1つもらってくる
    t->timeout = 0xffffffff;
    t->flags = TIMER_FLAGS_USING;
    t->next = 0;    // 作った時の番兵さんは一番後ろ
    timerctl.t0 = t;    // 番兵さんしかいないから先頭が番兵
    timerctl.next = 0xffffffff; // 番兵のタイムアウト時刻が次の時刻
    // timerctl.using = 1;
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
    int e;
    struct TIMER *t, *s;
    timer->timeout = timeout + timerctl.count;  // 今のカウント数と鳴らすまでを足してタイムアウトする予定時刻を入れる
    timer->flags = TIMER_FLAGS_USING;
    e = io_load_eflags();   // 割り込みを禁止しないと行けない
    io_cli();
    t = timerctl.t0;
    if (timer->timeout <= t->timeout) {
        // 先頭に入れるとき
        timerctl.t0 = timer;
        timer->next = t;    // 次のタイマはt
        timerctl.next = timer->timeout;
        io_store_eflags(e);
        return;
    }
    // どこにいれればいいか探す
    for (;;) {
        s = t;
        t = t->next;
        if (t == 0) {
            break;  // 一番後ろまでたどり着いたとき
        }
        if (timer->timeout <= t->timeout) {
            // sとtの間に入れる
            s->next = timer;    // sの次にTimerをセット
            timer->next = t;    // timerの次にtをセット
            io_store_eflags(e);
            return;
        }
    }
    return;
}

// タイマ割り込み(IRQ0)が発生したときに呼び出される割り込みハンドラ
// もしタイムアウトが設定されているならば呼び出されるたびにタイマーを減らしていく
void inthandler20(int *esp) {
    int i;
    char ts = 0;
    struct TIMER *timer;
    io_out8(PIC0_OCW2, 0x60);   // IRQ-00受付完了をPICに通知
    timerctl.count++;   // 1秒に100ずつ増えていくカウンタ(割り込み発生のたびに増える)

    if (timerctl.next > timerctl.count) {
        return; // まだ次の時刻になっていないから気にしない．
    }
    timer = timerctl.t0; // とりあえず先頭のタイマを入れる

    for (;;) {
        // timersのなかのタイマはすべて稼働中のもの
        if (timer->timeout > timerctl.count) {  // timeout -> 残り時間から予定時刻に
            break;
        }
        // タイムアウトしたタイマの処理
        timer->flags = TIMER_FLAGS_ALLOC;
        if (timer != mt_timer) {
            fifo32_put(timer->fifo, timer->data);
        } else {
            ts = 1; // mt_timerがタイムアウトしたとき
        }
        timer = timer->next;    // 次のタイマを代入
    }
    // i個目のタイマがタイムアウトしたからずらす
    timerctl.t0 = timer;
    // timerctl.nextの設定
    timerctl.next = timerctl.t0->timeout;
    // 全部の処理が終わってからタスクswitchしないと割り込みが混線する
    if (ts != 0) {
        mt_taskswitch();
    }
    return;
}
