CFLAGS = -march=i486 -m32 -nostdlib -fno-pic -fno-builtin
OBJS = bootpack.c nasmfunc.o ../lib/hankaku.c ../lib/mysprintf.c ../lib/dsctbl.c \
../lib/graphic.c ../lib/int.c ../lib/fifo.c ../lib/keyboard.c ../lib/mouse.c \
../lib/memory.c ../lib/sheet.c ../lib/timer.c mtask.c \
../lib/window.c console.c ../lib/file.c ../lib/stdlib.c
OSNAME   = helloOS
IMGNAME  = ../../haribote.img
VBOX     = /mnt/c/Program\ Files/Oracle/VirtualBox/VBoxManage.exe
NASMFILE = ../nasmfunc.asm
INCLUDE := bootpack.c
ASMAPPS := crack7.asm
CAPPS := a.c lines.c walk.c noodle.c beepdown.c color.c color2.c
APPS := $(patsubst %.asm,%.hrb,$(ASMAPPS)) $(patsubst %.c,%.hrb,$(CAPPS)) $(INCLUDE)
API_DIR = ./api
OBJS_API = $(patsubst %.asm,%.o,$(shell ls $(API_DIR)/*.asm)) # API_DIRのなかのファイル全部

# 別のディレクトリのソース達をもってくる
VPATH = src:../app/ src:../ src:./api/
# foreach内で改行を使うために...
define \n


endef

default :
	make img
	make clean

apilib.lib: $(OBJS_API) Makefile
	ar rcs $@ $(OBJS_API)

%.bin : %.asm Makefile
	nasm $< -o $*.bin -l $*.lst

%.o : %.asm Makefile
	nasm -g -f elf $< -o $@

# Cで書いたAppをコンパイルする
%.hrb: %.c apilib.lib ../app.ls Makefile
	gcc $< $(CFLAGS) -O0 -c -o $*.o
	ld -m elf_i386 -o $@ --oformat=binary -e __start -T ../app.ls $*.o apilib.lib

# ASMで書いたAppをつくる
%.hrb: %.asm apilib.lib ../app.ls Makefile
	nasm -g -f elf $< -o $*.o
	ld -m elf_i386 -o $@ --oformat=binary -e __start -T ../app.ls $*.o apilib.lib

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
	rm $(API_DIR)/*.o

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