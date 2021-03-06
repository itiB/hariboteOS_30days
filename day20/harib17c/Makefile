CFLAGS = -march=i486 -m32 -nostdlib -fno-pic -fno-builtin
OBJS = bootpack.c nasmfunc.o ../lib/hankaku.c ../lib/mysprintf.c ../lib/dsctbl.c \
../lib/graphic.c ../lib/int.c ../lib/fifo.c ../lib/keyboard.c ../lib/mouse.c \
../lib/memory.c ../lib/sheet.c ../lib/timer.c ../lib/mtask.c \
../lib/window.c console.c ../lib/file.c ../lib/stdlib.c
OSNAME   = helloOS
IMGNAME  = ../../haribote.img
VBOX     = /mnt/c/Program\ Files/Oracle/VirtualBox/VBoxManage.exe
NASMFILE = ../nasmfunc.asm
APP = hlt.hrb

default :
	make img
	make clean

%.bin : %.asm Makefile
	nasm $*.ams -o $*.bin -l $*.lst

ipl10.bin : ../ipl10.asm Makefile
	nasm ../ipl10.asm -o ipl10.bin -l ipl10.lst

asmhead.bin : ../asmhead.asm Makefile
	nasm ../asmhead.asm -o asmhead.bin -l asmhead.lst

hlt.hrb: hlt.asm Makefile
	nasm hlt.asm -o hlt.hrb -l hlt.lst

nasmfunc.o : $(NASMFILE) Makefile
	nasm -g -f elf $(NASMFILE) -o nasmfunc.o

bootpack.hrb : ../har.ld $(OBJS) Makefile
	gcc -Wl,-Map=haribote.map $(CFLAGS) -T ../har.ld $(OBJS) -o bootpack.hrb

haribote.sys : asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > haribote.sys

haribote.img : $(APP) ipl10.bin haribote.sys Makefile
	mformat -f 1440 -C -B ipl10.bin -i haribote.img ::
	mcopy -i haribote.img haribote.sys ::
	mcopy $(APP) -i $@ ::
	mv haribote.img ../../

# bootpack.bin: $(C_OBJS)
# 	ld -m elf_i386 -o bootpack.bin -T ../har.ld -e HariMain --oformat=binary -Map=os.map $^

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

# ld: リンカ
# $^: 依存ファイルのリスト
# ld -m elf_i386 -o os.bin -T scripts/bootpack.lds -e HariMain --oformat=binary -Map=os.map $^