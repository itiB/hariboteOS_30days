extern void io_hlt(void);
// extern void write_mem8(int addr, int data);

void HariMain(void)
{
    int i;      // 変数宣言

    // 追加
    char *p;    // Byteのポインタ変数を用意
    p = (char * ) 0xa0000;

    for (i = 0; i <= 0xffff; i++ ) {
        *(p + i) = i & 0x0f;
    }
    for ( ; ; ) {
        io_hlt();
    }
}