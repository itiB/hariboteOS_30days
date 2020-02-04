#include "bootpack.h"

void init_gdtidt(void) {
    // // とりあえず0x270000-0x27ffff番地をGDTにする
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) 0x00270000;
    // // 0x26f800-0x26ffff
    struct GATE_DESCRIPTOR *idt = (struct GATE_DESCRIPTOR *) 0x0026f800;
    int i;

    // セグメント : 切り分けられたブロックのこと

    // GDTの初期化, 定義されうるセグメント(8192個)すべて
    for (i = 0; i < 8192; i++) {
        // limit = 0, base = 0, access = 0
        set_segmdesc(gdt + i, 0, 0, 0);
    }
    // 第1セグメントの設定
    //  limit = 0xffffffff : セグメントの大きさ(4G)
    //  base = 0 : 始まりの番地
    //  access = 0x4092
    set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, 0x4092);
    // 第2セグメントの設定
    //  limit 0x7ffff : 512KB => bootpack.hrbを実行するためのもの
    set_segmdesc(gdt + 2, 0x0007ffff, 0x00280000, 0x409a);
    // GDTLへの代入
    load_gdtr(0xffff, 0x0026f800);

    // IDTの初期化, 割り込み番号0-256分を
    for (i = 0; i < 256; i++) {
        set_gatedesc(idt + i, 0, 0, 0);
    }
    load_idtr(0x7ff, 0x0026f800);

    return;
}

void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar) {
    if (limit > 0xfffff) {
        ar |= 0x8000;       // G_bit = 1
        limit /= 0x1000;
    }
    sd -> limit_low     = limit & 0xffff;           // 20bit セグメントの大きさ
    sd -> base_low      = base & 0xffff;            // base: low + mid + high 32bit ベースアドレス
    sd -> base_mid      = (base >> 16) & 0xff;
    sd -> access_right  = ar & 0xff;                // access_right -> ar セグメント属性
    sd -> limit_high    = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
    sd -> base_high     = (base >> 24) & 0xff;
    return;
}

void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar) {
    gd -> offset_low    = offset & 0xffff;
    gd -> selector      = selector;
    gd -> dw_count      = (ar >> 8) & 0xff;
    gd -> access_right  = ar & 0xff;
    gd -> offset_high   = (offset >> 16) & 0xffff;
    return;
}