#include "bootpack.h"

// ディスクイメージ内のFAT圧縮をとく
void file_readfat(int *fat, unsigned char *img) {
    int i, j = 0;
    for (i = 0; i < 2880; i += 2) {
        fat[i + 0] = (img[j + 0]      | img[j + 1] << 8) & 0xfff;   // 0byte  + 1byte(前半8bit)
        fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;
        j += 3;
    }
    return;
}

void file_loadfile(int clustno, int size, char *buf, int *fat, char *img) {
    int i;
    for (;;) {
        if (size <= 512) {
            for (i = 0; i < size; i++) {
                buf[i] = img[clustno * 512 + i];
            }
            break;
        }
        for (i = 0; i < 512; i++) {
            buf[i] = img[clustno * 512 + i];
        }
        size -= 512;
        buf += 512;
        clustno = fat[clustno];
    }
    return;
}

struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max) {
    int i, j = 0;
    char s[12];
    // file名を初期化
    for (i = 0; i < 11; i++) { s[i] = ' '; }
    for (i = 0; name[i] != 0; i++) {
        if (name[i] == '.' && j <= 8) {
            j = 8;
        } else {
            s[j] = name[i];
            if ('a' <= s[j] && s[j] <= 'z') {
                // 小文字を大文字に直す
                s[j] -= 0x20;
            }
            j++;
        }
    }
    // ファイルを探す
    for (i = 0; i < 244; ) {
        if (finfo[i].name[0] == 0x00) {
            break;
        }
        if ((finfo[i].type & 0x18) == 0) {
            for (j = 0; j < 11; j++) {
                if (finfo[i].name[j] != s[j]) {
                    goto next;
                }
            }
            return finfo + i;   // ファイルが見つかった
        }
next:
        i++;
    }
    return 0;   // 見つからなかった
}
