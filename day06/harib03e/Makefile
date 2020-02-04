CFLAGS = -march=i486 -m32 -nostdlib -fno-pic -fno-builtin -fno-stack-protector
OBJS = bootpack.c nasmfunc.o ../hankaku.c ../mysprintf.c dsctbl.c graphic.c int.c

default :
	make clean

%.bin : %.asm Makefile
	nasm $*.ams -o $*.bin -l $*.lst

ipl10.bin : ../ipl10.asm Makefile
	nasm ../ipl10.asm -o ipl10.bin -l ipl10.lst

asmhead.bin : ../asmhead.asm Makefile
	nasm ../asmhead.asm -o asmhead.bin -l asmhead.lst

# %.o : %.asm Makefile
# 	nasm -g -f -elg $*.asm -o $*.o

nasmfunc.o : ../nasmfunc.asm Makefile
	nasm -g -f elf ../nasmfunc.asm -o nasmfunc.o

mysprintf.o: mysprintf.c
	gcc -c $(CFLAGS) -o mysprintf.o mysprintf.c

%.o : %.c Makefile
	gcc -c $(CFLAGS) -o $*.o $*.c

bootpack.hrb : ../har.ld $(OBJS) Makefile
	gcc $(CFLAGS) -T ../har.ld $(OBJS) -o bootpack.hrb

haribote.sys : asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > haribote.sys

haribote.img : ipl10.bin haribote.sys Makefile
	mformat -f 1440 -C -B ipl10.bin -i haribote.img ::
	mcopy -i haribote.img haribote.sys ::
	mv haribote.img ../../

asm :
	make -r ipl10.bin

img :
	make -r haribote.img

clean :
	make -r haribote.img
	rm *.bin *.lst *.hrb *.sys *.o