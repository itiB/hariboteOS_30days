#include "../apilib.h"

unsigned char rgb2pal(int r, int g, int b, int x, int y);

void HariMain(void) {
    char *buf;
    int win, x, y;
    api_initmalloc();
    buf = api_malloc(144 * 164);
    win = api_openwin(buf, 144, 164, -1, "color");
    for (y = 0; y < 128; y++) {
        for (x = 0; x < 128; x++) {
            buf[(x + 8) + (y + 28) * 144] = rgb2pal(x * 2, y * 2, 0, x, y);
        }
    }
    api_refreshwin(win, 8, 28, 136, 156);
    api_getkey(1);  // 適当な入力をまつ
    api_end();
}

// 滑らかにするために画素パズルする
unsigned char rgb2pal(int r, int g, int b, int x, int y) {
    static int table[4] = { 3, 1, 0, 2 };
    int i;
    x &= 1; // 偶数か奇数か
    y &= 1;
    i = table[x + y * 2]; // 中間色を作るため
    r = (r * 21) / 256; // 0-20にする
    g = (g * 21) / 256;
    b = (b * 21) / 256;
    r = (r + i) / 4; // 0-5にする
    g = (g + i) / 4;
    b = (b + i) / 4;
    return 16 + r + g * 6 + b * 36;
}