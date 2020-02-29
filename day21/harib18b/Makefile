CFLAGS = -march=i486 -m32 -nostdlib -fno-pic -fno-builtin
OBJS = bootpack.c nasmfunc.o ../lib/hankaku.c ../lib/mysprintf.c ../lib/dsctbl.c \
../lib/graphic.c ../lib/int.c ../lib/fifo.c ../lib/keyboard.c ../lib/mouse.c \
../lib/memory.c ../lib/sheet.c ../lib/timer.c ../lib/mtask.c \
../lib/window.c console.c ../lib/file.c ../lib/stdlib.c
OSNAME   = helloOS
IMGNAME  = ../../haribote.img
VBOX     = /mnt/c/Program\ Files/Oracle/VirtualBox/VBoxManage.exe
NASMFILE = nasmfunc.asm
INCLUDE := hello.asm hello2.asm bootpack.c a.hrb hello3.hrb
APPS := $(patsubst %.asm,%.hrb,$(INCLUDE))

# foreach内で改行を使うために...
define \n


endef

a.hrb: a.c a_asm.asm app.ls Makefile
	nasm a_asm.asm -f elf32 -o a_asm.o
	gcc a.c $(CFLAGS) -O0 -c -o a.o
	ld -m elf_i386 -o $@ --oformat=binary -e __start -T app.ls a_asm.o a.o

hello3.hrb: hello3.c a_asm.asm app.ls Makefile
	nasm a_asm.asm -f elf32 -o a_asm.o
	gcc hello3.c $(CFLAGS) -O0 -c -o hello3.o
	ld -m elf_i386 -o $@ --oformat=binary -e __start -T app.ls a_asm.o hello3.o

# Appをつくるためのルール
%.hrb: %.asm Makefile
	nasm $< -o $@

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
	nasm -g -f elf $(NASMFILE) -o $@

bootpack.hrb : ../har.ld $(OBJS) Makefile
	gcc $(CFLAGS) -T ../har.ld $(OBJS) -o bootpack.hrb

haribote.sys : asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > haribote.sys

haribote.img : $(APPS) ipl10.bin haribote.sys Makefile
	mformat -f 1440 -C -B ipl10.bin -i haribote.img ::
	mcopy -i haribote.img haribote.sys ::
	$(foreach FILE,$(APPS),mcopy $(FILE) -i $@ ::$(\n))
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