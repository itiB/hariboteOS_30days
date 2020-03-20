CFLAGS = -march=i486 -m32 -nostdlib -fno-pic -fno-builtin
OBJS = bootpack.c nasmfunc.o hankaku.c mysprintf.c dsctbl.c graphic.c int.c \
	fifo.c keyboard.c mouse.c memory.c sheet.c timer.c mtask.c window.c \
	console.c file.c stdlib.c

default :
	make ipl10.bin
	make haribote.sys

# 一般
%.bin : %.asm Makefile
	nasm $< -o $*.bin -l $*.lst

%.o : %.asm Makefile
	nasm -g -f elf $< -o $@

bootpack.hrb : har.ld $(OBJS) Makefile
	gcc $(CFLAGS) -T har.ld $(OBJS) -o bootpack.hrb

haribote.sys : asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > haribote.sys

# コマンドたち
clean:
	rm -f *.lst *.o asmhead.bin *.hrb

src_only:
	make -r clean
	rm -f *.bin
	rm -f *.sys
