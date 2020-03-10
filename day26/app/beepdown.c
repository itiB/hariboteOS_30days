int api_getkey(int mode);
int api_alloctimer(void);
void api_inittimer(int timer, int data);
void api_settimer(int timer, int time);
void api_beep(int tone);
void api_end(void);

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