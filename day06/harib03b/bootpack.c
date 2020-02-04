#include<bootpack.h>

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
    char s[40];
    char mcursor[256];
    int mx = 120, my =120;

    init_palette();
    init_screen(binfo -> vram, binfo -> scrnx, binfo -> scrny);

    putfont8_asc(binfo -> vram, binfo -> scrnx,  9,  9, COL8_000000, "Haribote OS.");
    putfont8_asc(binfo -> vram, binfo -> scrnx,  8,  8, COL8_FFFFFF, "Haribote OS.");

    sprintf(s, "scrnx = %d", binfo -> scrnx);
    putfont8_asc(binfo -> vram, binfo -> scrnx, 16, 64, COL8_FFFFFF, s);

    init_mouse_coursor8(mcursor, COL8_008484);
    putblock8_8(binfo -> vram, binfo -> scrnx, 16, 16, mx, my, mcursor, 16);

    for ( ; ; ) {
        io_hlt();
    }
}
