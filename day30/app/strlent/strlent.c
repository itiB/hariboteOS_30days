#include "../apilib.h"
#include "../stdlib.h"

void HariMain(void) {
    int i;
    int len = _strlen("hello");
    for (i = 0; i < len; i++) {
        api_putstr0("* ");
    }
    api_end();
}