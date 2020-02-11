#include "bootpack.h"

// タイマ割り込み(IRQ0)が発生したときに呼び出される割り込みハンドラ
void inthandler20(int *esp) {
    io_out8(PIC0_OCW2, 0x60);   // IRQ-00受付完了をPICに通知
    return;
}