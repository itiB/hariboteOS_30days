#include "bootpack.h"

// FIFOバッファの初期化関数
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task) {
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;  // 空き
    fifo->flags = 0;
    fifo->p = 0;    // 書き込み位置
    fifo->q = 0;    // 読み込み位置
    fifo->task = task;  // データが入ったときに起こすタスク
    return;
}

// FIFOに32bit(1byte)記録する
int fifo32_put(struct FIFO32 *fifo, int data) {
    if (fifo->free == 0) {
        // 空きがなくてあふれた状態->return -1
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->p] = data;
    fifo->p++;
    // サイズを超えたら0に戻ってループする
    if (fifo->p == fifo->size) {
        fifo->p = 0;
    }
    fifo->free--;

    // もし起こすタスクがあるならば起こす
    if (fifo->task != 0) {
        if (fifo->task->flags != 2) { // もしタスクが寝ているならば
            task_run(fifo->task, -1, 0);   // 起こす
        }
    }
    return 0;
}

// fifo から1byteもらってくる
int fifo32_get(struct FIFO32 *fifo) {
    int data;
    if (fifo->free == fifo->size) {
        // バッファが空の時にはとりあえず-1
        return -1;
    }
    data = fifo->buf[fifo->q];
    fifo->q++;
    if (fifo->q == fifo->size) {
        fifo->q = 0;
    }
    fifo->free++;
    return data;
}

// どのくらいデータがたまっているか報告
int fifo32_status(struct FIFO32 *fifo) {
    return fifo->size - fifo->free;
}
