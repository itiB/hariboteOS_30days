extern void io_hlt(void);
// extern void write_mem8(int addr, int data);

void HariMain(void)
{
    int i;      // 変数宣言

    // 追加
    char *p;    // Byteのポインタ変数を用意

    for (i = 0xa0000; i <= 0xaffff; i++ ) {
        // write_mem8(i, i & 0x0f);  // MOV BYTE [i] 15

        //   書き直してみる
        // *i = i = 0x0f;         // [i]がByteなのかWordなのかわからない
        p = (char * ) i;
        *p = i & 0x0f;
    }
    for ( ; ; ) {
        io_hlt();
    }
}