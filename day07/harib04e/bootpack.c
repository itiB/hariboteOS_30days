#include "bootpack.h"

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
    char s[40];
    char mcursor[256];
    char keybuf[32];
    int mx = 120, my =120, i, j;

    init_gdtidt();
    init_pic();
    io_sti();

    init_palette();
    init_screen(binfo -> vram, binfo -> scrnx, binfo -> scrny);

    putfont8_asc(binfo -> vram, binfo -> scrnx,  9,  9, COL8_000000, "Haribote OS.");
    putfont8_asc(binfo -> vram, binfo -> scrnx,  8,  8, COL8_FFFFFF, "Haribote OS.");

    sprintf(s, "scrnx = %d", binfo -> scrnx);
    putfont8_asc(binfo -> vram, binfo -> scrnx, 16, 64, COL8_FFFFFF, s);

    init_mouse_coursor8(mcursor, COL8_008484);
    putblock8_8(binfo -> vram, binfo -> scrnx, 16, 16, mx, my, mcursor, 16);

    io_out8(PIC0_IMR, 0xf9); /* pic1とキーボード許可 */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可 */

    fifo8_init(&keyfifo, 32, keybuf);

    for ( ; ; ) {
        io_cli();

        if (fifo8_status(&keyfifo) == 0) {
            io_stihlt();
        } else {
            i = fifo8_get(&keyfifo);
            io_sti();
            sprintf(s, "%x", i);
            boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
            putfont8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
        }
    }
}
