#include "../apilib.h"

void HariMain(void) {
    int i, timer;
    timer = api_alloctimer();
    api_inittimer(timer, 128);
    for (i = 20000000; i >= 2000; i -= i / 100) {
        // 20KHz ~ 20Hz: 人間に聴こえる範囲
        // i は 1%ずつ減らされていく
        api_beep(i);
        api_settimer(timer, 1);
        if (api_getkey(1) != 128) {
            break;
        }
    }
    api_beep(0);
    api_end();
}