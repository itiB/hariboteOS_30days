#include "bootpack.h"

int mt_tr;

// タスクの初期設定．
//  戻り値: タスクのアドレス
struct TASK *task_init(struct MEMMAN *memman) {
    int i;
    struct TASK *task;
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof(struct TASKCTL));
    for (i = 0; i < MAX_TASKS; i ++) {
        taskctl->tasks0[i].flags = 0;
        taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
        set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
    }
    task = task_alloc();
    task->flags = 2;    // 動作中マーク
    taskctl->running = 1;
    taskctl->now = 0;
    taskctl->tasks[0] = task;
    load_tr(task->sel);
    task_timer = timer_alloc();
    timer_settime(task_timer, 2);
    return task;
}

struct TASK *task_alloc(void) {
    int i;
    struct TASK *task;
    for (i = 0; i < MAX_TASKS; i++) {
        if (taskctl->tasks0[i].flags == 0) {
            task = &taskctl->tasks0[i];
            task->flags      = 1;
            task->tss.eflags = 0x00000202;  // IF = 1;
            task->tss.eax    = 0;   // まずはとりあえず0
            task->tss.ecx    = 0;
            task->tss.edx    = 0;
            task->tss.ebx    = 0;
            // task->tss.esp
            task->tss.ebp    = 0;
            task->tss.esi    = 0;
            task->tss.edi    = 0;
            task->tss.es     = 0;
            task->tss.cs     = 0;
            task->tss.ds     = 0;
            task->tss.fs     = 0;
            task->tss.gs     = 0;
            task->tss.ldtr   = 0;
            task->tss.icmap  = 0x40000000;
            return task;
        }
    }
    return 0;   // もう全部使っちゃてるよ～
}

// タスクを走らせる
void task_run(struct TASK *task) {
    task->flags = 2;
    taskctl->tasks[taskctl->running] = task;
    taskctl->running++;
    return;
}

// taskを切り替える
void task_switch(void) {
    timer_settime(task_timer, 2);
    // もしタスクが1つしかないなら切り替える必要がない
    if (taskctl->running >= 2) {
        taskctl->now++;
        // もし一番後ろのタスクまで行ったらはじめのタスクまで戻す
        if (taskctl->now == taskctl->running) {
            taskctl->now = 0;
        }
        farjmp(0, taskctl->tasks[taskctl->now]->sel);
    }
    return;
}

// タスクが来ていないときスリープする
void task_sleep(struct TASK *task) {
    int i;
    char ts = 0;
    if (task->flags == 2) { // 指定したタスクがもし動いていたならば
        if (task == taskctl->tasks[taskctl->now]) {
            ts = 1; // 自分自身を寝かせる，あとでタスクswitchさせるために記覚えておく
        }
        // taskがどこにいるかを探す
        for (i = 0; i < taskctl->running; i++) {
            if (taskctl->tasks[i] == task) {
                // 見つけた！
                break;
            }
        }
        taskctl->running--;
        if (i < taskctl->now) {
            taskctl->now--; // ずれるからこっちも合わせる
        }
        // ずらし作業
        for (; i < taskctl->running; i++) {
            taskctl->tasks[i] = taskctl->tasks[i + 1];
        }
        // タスクを動作していないことにする
        task->flags = 1;
        if (ts != 0) {
            // タスクswitchする
            if (taskctl->now >= taskctl->running) {
                // nowがおかしな値になっていた場合には修正
                taskctl->now = 0;
            }
            farjmp(0, taskctl->tasks[taskctl->now]->sel);
        }
    }
    return;
}


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