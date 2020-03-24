#include "bootpack.h"

struct FIFO32 *keyfifo;
int keydata0;

// キーボードコントローラがデータ送信可能になるのを待つ(制御回路)
void wait_KBC_sendready(void) {
    // 命令CPUから送りすぎると回路が処理しきれないから受け付けるまで待つ
    for (;;) {
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
            break;
        }
    }
    return;
}

// キーボードコントローラの初期化
void init_keyboard(struct FIFO32 *fifo, int data0) {
    // 書き込み先のバッファを記憶
    keyfifo = fifo;
    keydata0 = data0;
    // コントローラーの初期化
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}

// PS/2キーボードからの割り込み
void inthandler21(int *esp) {
    unsigned char data;
    io_out8(PIC0_OCW2, 0x61);   // IRQ-01受付完了をPICに通知, 再度監視刺せる
    data = io_in8(PORT_KEYDAT); // fifo8にdataをのっける
    fifo32_put(keyfifo, data + keydata0);
    return;
}