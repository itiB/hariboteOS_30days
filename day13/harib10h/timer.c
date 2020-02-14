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
    int e;
    struct TIMER *t, *s;
    timer->timeout = timeout + timerctl.count;  // 今のカウント数と鳴らすまでを足してタイムアウトする予定時刻を入れる
    timer->flags = TIMER_FLAGS_USING;
    e = io_load_eflags();   // 割り込みを禁止しないと行けない
    io_cli();
    timerctl.using++;
    if (timerctl.using == 1) {
        // 動作中のタイマがこれ1つのとき
        timerctl.t0 = timer;
        timer->next = 0;    // 次はない
        timerctl.next = timer->timeout;
        io_store_eflags(e);
        return;
    }
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
    // 一番後ろに入れる
    s->next = timer;
    timer->next = 0;
    io_store_eflags(e); // 割り込みを許可
    return;
}

// タイマ割り込み(IRQ0)が発生したときに呼び出される割り込みハンドラ
// もしタイムアウトが設定されているならば呼び出されるたびにタイマーを減らしていく
void inthandler20(int *esp) {
    int i;
    struct TIMER *timer;
    io_out8(PIC0_OCW2, 0x60);   // IRQ-00受付完了をPICに通知
    timerctl.count++;   // 1秒に100ずつ増えていくカウンタ(割り込み発生のたびに増える)

    if (timerctl.next > timerctl.count) {
        return; // まだ次の時刻になっていないから気にしない．
    }
    timer = timerctl.t0; // とりあえず先頭のタイマを入れる

    for (i = 0; i < timerctl.using; i++) {
        // timersのなかのタイマはすべて稼働中のもの
        if (timer->timeout > timerctl.count) {  // timeout -> 残り時間から予定時刻に
            break;
        }
        // タイムアウトしたタイマの処理
        timer->flags = TIMER_FLAGS_ALLOC;
        fifo32_put(timer->fifo, timer->data);
        timer = timer->next;
    }
    timerctl.using -= i;
    // i個目のタイマがタイムアウトしたからずらすNEW!
    timerctl.t0 = timer;
    // timerctl.nextの設定
    if (timerctl.using > 0) {
        timerctl.next = timerctl.t0->timeout;
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