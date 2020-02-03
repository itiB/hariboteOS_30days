#include<stdio.h>

int main() {
    int i;
    char str[10];

    for (i = 0; i < 3; i++) {
        sprintf(str, "%d\n", i);
    }
    printf(str);
}

// ❯ ldd a.out
//         linux-vdso.so.1 (0x00007ffff4f7c000)
//         libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007fbfbe340000)
//         /lib64/ld-linux-x86-64.so.2 (0x00007fbfbea00000