// mysprintf.c
// http://bttb.s1.valueserver.jp/wordpress/blog/2017/12/17/makeos-5-2/

#include <stdarg.h>

int dec2asc (char *str, int dec);
int hex2asc (char *str, int dec);
void _sprintf (char *str, char *fmt, ...);
int _strlen(char *str);
int _strncmp(char *s1, char *s2, unsigned int n);
int _isdigit(char c);
int _strtol(char *s, char **endp, int base);

//10進数からASCIIコードに変換
int dec2asc (char *str, int dec) {
    int len = 0, len_buf; //桁数
    int buf[10];
    while (1) { //10で割れた回数（つまり桁数）をlenに、各桁をbufに格納
        buf[len++] = dec % 10;
        if (dec < 10){
            break;
        }
        dec /= 10;
    }
    len_buf = len;
    while (len) {
        *(str++) = buf[--len] + 0x30;
    }
    return len_buf;
}

//16進数からASCIIコードに変換
int hex2asc (char *str, int dec) { //10で割れた回数（つまり桁数）をlenに、各桁をbufに格納
    int len = 0, len_buf; //桁数
    int buf[10];
    while (1) {
        buf[len++] = dec % 16;
        if (dec < 16) break;
        dec /= 16;
    }
    len_buf = len;
    while (len) {
        len --;
        *(str++) = (buf[len]<10)?(buf[len] + 0x30):(buf[len] - 9 + 0x60);
    }
    return len_buf;
}

void _sprintf (char *str, char *fmt, ...) {
    va_list list;
    int i, len;
    va_start (list, fmt);

    while (*fmt) {
        if(*fmt=='%') {
            fmt++;
            switch(*fmt){
                case 'd':
                    len = dec2asc(str, va_arg (list, int));
                    break;
                case 'x':
                    len = hex2asc(str, va_arg (list, int));
                    break;
            }
            str += len; fmt++;
        } else {
            *(str++) = *(fmt++);
        }
    }
    *str = 0x00; //最後にNULLを追加
    va_end (list);
}

int _strlen(char *str) {
    char *tmp = str;
    while ('\0' != *str)
        str++;

    return str - tmp;
}

int _strncmp(char *s1, char *s2, unsigned int n) {
    unsigned int i = 0;
    while(*s1 != '\0' && *s2 != '\0' && ++i < n) {
        if(*s1 != *s2) break;
        s1++;
        s2++;
    }
    return(*s1 - *s2);
}

int _isdigit(char c) {
    return((c >= '0' && c <= '9'));
}

// 文字列の数値変換
// お借りした <https://github.com/kamaboko123/30daysOS/blob/develop/tools/stdlibc/src/stdlibc.c>
int _strtol(char *s, char **endp, int base) {
    int _base;
    int ret = 0;
    int sign = 0;

    //空白のスキップ
    while(*s == ' ') s++;

    if(*s == '-') sign = 1;

    //base = 0なら基数は渡された文字列の表記に従う
    if(base == 0){
        //渡された文字列の表記方法の検出
        //16進数
        if(_strncmp(s, "0x", 2) == 0 || _strncmp(s, "0X", 2) == 0){
            _base = 16;
        }
        //8進数
        if(_strncmp(s, "0", 1) == 0){
            _base = 8;
        }
        //それ以外は10進数
        else{
            _base = 10;
        }
    }
    else{
        _base = base;
    }

    //16進数
    if(_base == 16){
       while(_isdigit(*s) || (*s >= 'a' && *s <= 'f') || (*s >= 'A' && *s <= 'F') ){
            if(_isdigit(*s)){
                ret = (ret * 16) + (*s - '0');
            }
            else if(*s >= 'a' && *s <= 'f'){
                ret = (ret * 16) + (*s - 'a' + 10);
            }
            else if(*s >= 'A' && *s <= 'F'){
                ret = (ret * 16) + (*s - 'A' + 10);
            }
            s++;
        }
    }

    //8進数（多分使わないので省略）

    //10進数
    if(_base == 10){
        while(_isdigit(*s)){
            ret = (ret * 10) + (*s - '0');
            s++;
        }
    }

    if(sign != 0) ret *= -1;
    *endp = s;
    return ret;
}