CFLAGS = -march=i486 -m32 -nostdlib -fno-pic -fno-builtin
OBJS = bootpack.c nasmfunc.o ../hankaku.c ../mysprintf.c ../dsctbl.c ../graphic.c \
../int.c ../fifo.c ../keyboard.c ../mouse.c ../memory.c ../sheet.c ../timer.c ../mtask.c
OSNAME   = helloOS
IMGNAME  = ../../haribote.img
VBOX     = /mnt/c/Program\ Files/Oracle/VirtualBox/VBoxManage.exe
NASMFILE = ../nasmfunc.asm
APP = bootpack.c

default :
	make img
	make clean

%.bin : %.asm Makefile
	nasm $*.ams -o $*.bin -l $*.lst

ipl10.bin : ../ipl10.asm Makefile
	nasm ../ipl10.asm -o ipl10.bin -l ipl10.lst

asmhead.bin : ../asmhead.asm Makefile
	nasm ../asmhead.asm -o asmhead.bin -l asmhead.lst

nasmfunc.o : $(NASMFILE) Makefile
	nasm -g -f elf $(NASMFILE) -o nasmfunc.o

bootpack.hrb : ../har.ld $(OBJS) Makefile
	gcc $(CFLAGS) -T ../har.ld $(OBJS) -o bootpack.hrb

haribote.sys : asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > haribote.sys

haribote.img : ipl10.bin haribote.sys Makefile
	mformat -f 1440 -C -B ipl10.bin -i haribote.img ::
	mcopy -i haribote.img haribote.sys ::
	mcopy $(APP) -i $@ ::
	mv haribote.img ../../

asm :
	make -r ipl10.bin

img :
	make -r haribote.img

clean :
	rm *.bin *.lst *.hrb *.sys *.o

# VirtualBox
vdel:
	$(VBOX) unregistervm $(OSNAME) --delete

vrun:
	$(VBOX) createvm --name $(OSNAME) --ostype Other --register
	$(VBOX) modifyvm $(OSNAME) --memory 32
	$(VBOX) storagectl $(OSNAME) --name Floppy --add floppy
	$(VBOX) storageattach $(OSNAME) --storagectl Floppy --device 0 --medium $(IMGNAME)
	$(VBOX) startvm $(OSNAME)

run :
	make img
	make clean
	make vdel
	make vrun
