#include "bootpack.h"

int mt_tr;

// タスクの初期設定．
//  戻り値: タスクのアドレス
struct TASK *task_init(struct MEMMAN *memman) {
    int i;
    struct TASK *task, *idle;
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof(struct TASKCTL));
    for (i = 0; i < MAX_TASKS; i ++) {
        taskctl->tasks0[i].flags = 0;
        taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
        set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
    }

    for (i = 0; i < MAX_TASKLEVELS; i++) {
        taskctl->level[i].running = 0;
        taskctl->level[i].now = 0;
    }
    task = task_alloc();
    task->flags = 2;    // 動作中マーク
    task->priority = 2; // 0.02秒で入れ替える
    task->level = 0; // はじめはlevel 0の最優先にしておく
    task_add(task);
    task_switchsub();   // レベル設定
    load_tr(task->sel);
    task_timer = timer_alloc();
    timer_settime(task_timer, task->priority);

    // idleを最低レベルで実行する
    idle = task_alloc();
    idle->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
    idle->tss.eip = (int) &task_idle;
    idle->tss.es  = 1 * 8;
    idle->tss.cs  = 2 * 8;
    idle->tss.ss  = 1 * 8;
    idle->tss.ds  = 1 * 8;
    idle->tss.fs  = 1 * 8;
    task_run(idle, MAX_TASKLEVELS - 1, 1);

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

// タスクを走らせる，タスクの優先度を変える
void task_run(struct TASK *task, int level, int priority) {
    if (level < 0) {
        level = task->level;    // レベル変更しない
    }
    if (priority > 0) {
        task->priority = priority;
    }

    if (task->flags == 2 && task->level != level) { // 動作中のレベル変更
        task_remove(task);  // これを実行するとFlagは1に勝手になってしたのifも実行される
    }

    if (task->flags != 2) {
        // スリープから起こされるとき
        task->level = level;
        task_add(task);
    }
    taskctl->lv_change = 1;  // 次回のタスクswitchのときにレベルを見直す

    return;
}

// taskを切り替える
void task_switch(void) {
    struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
    struct TASK *new_task, *now_task = tl->tasks[tl->now];
    tl->now++;
    if (tl->now == tl->running) {
        tl->now = 0;
    }
    if (taskctl->lv_change != 0) {
        task_switchsub();
        tl = &taskctl->level[taskctl->now_lv];
    }
    new_task = tl->tasks[tl->now];
    timer_settime(task_timer, new_task->priority);
    if (new_task != now_task) {
        farjmp(0, new_task->sel);
    }
    return;
}

// タスクが来ていないときスリープする
void task_sleep(struct TASK *task) {
    struct TASK *now_task;

    if (task->flags == 2) {
        // 指定したタスクがもし動いていたならば
        now_task = task_now();
        task_remove(task);  // これを実行するとFlagは1になる
        if (task == now_task) {
            // 自分自身のスリープにはタスクswitchが必要
            task_switchsub();
            now_task = task_now();  // 設定後の現在のタスクを教えてもらう
            farjmp(0, now_task->sel);
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

// 現在動作中のstruct TASKの番地を教えてくれる
struct TASK *task_now(void) {
    struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
    return tl->tasks[tl->now];
}

// struct TASKLEVELに追加する
void task_add(struct TASK *task) {
    struct TASKLEVEL *tl = &taskctl->level[task->level];
    tl->tasks[tl->running] = task;
    tl->running++;
    task->flags = 2;    // 動作中
    return;
}

void task_remove(struct TASK *task) {
    int i;
    struct TASKLEVEL *tl = &taskctl->level[task->level];

    // タスクのいる場所を探す
    for (i = 0; i < tl->running; i++) {
        if (tl->tasks[i] == task) {
            break;
        }
    }
    tl->running--;
    if (i < tl->now) {
        tl->now--;  // ずれるので合わせる
    }
    if (tl->now >= tl->running) {
        // nowがおかしな値になっていたら修正
        tl->now = 0;
    }
    task->flags = 1;    // スリープ中

    // ずらし
    for (; i < tl->running; i++) {
        tl->tasks[i] = tl->tasks[i + 1];
    }
    return;
}

// タスクswitchの際にどのレベルにするか決める
void task_switchsub(void) {
    int i ;
    // 一番上のレベルを探す
    for (i = 0; i < MAX_TASKLEVELS; i++) {
        if (taskctl->level[i].running > 0) {
            // 見つけた！
            break;
        }
    }
    taskctl->now_lv = i;
    taskctl->lv_change = 0;
    return;
}

// やることがなくなったらこのタスクにスイッチ，hltしてくれる
void task_idle(void) {
    for(;;) {
        io_hlt();
    }
}