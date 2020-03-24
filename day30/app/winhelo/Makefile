APP = winhelo

CFLAGS = -march=i486 -m32 -nostdlib -fno-pic -fno-builtin
APILIBPATH = ../../apilib
HARIBOTEPATH = ../../haribote

OSNAME   = helloOS
IMGNAME  = ../../../../haribote.img
VBOX     = /mnt/c/Program\ Files/Oracle/VirtualBox/VBoxManage.exe

default:
	make $(APP).hrb

# ファイル生成
$(APP).hrb: $(APP).o $(APILIBPATH)/apilib.lib ../app_large.ls Makefile ../app_make.txt
	ld -m elf_i386 -o $@ --oformat=binary -e __start -T ../app_large.ls $(APP).o $(APILIBPATH)/apilib.lib

haribote.img : $(HARIBOTEPATH)/ipl20.bin $(HARIBOTEPATH)/haribote.sys $(APP).hrb \
Makefile ../app_make.txt
	mformat -f 1440 -C -B $(HARIBOTEPATH)/ipl20.bin -i $@ ::
	mcopy -i $@ $(HARIBOTEPATH)/haribote.sys ::
	mcopy $(APP).hrb -i $@ ::

# 一般規則
%.o : %.asm Makefile
	nasm -g -f elf $< -o $@

%.o : %.c ../apilib.h Makefile
	gcc $< $(CFLAGS) -O0 -c -o $@

# コマンド
run :
	make haribote.img
	mv haribote.img ../../../../
	make vdel
	make vrun

full :
	make -C $(APILIBPATH)
	make $(APP).hrb

# api_libのmakeもHariboteのmakeもやる
run_full :
	make -C $(APILIBPATH)
	make -C $(HARIBOTEPATH)
	make run

clean :
	rm -f *.lst *.o *.bin haribote.img

src_only :
	make clean
	rm -f $(APP).hrb

# VirtualBox
vdel:
	$(VBOX) unregistervm $(OSNAME) --delete

vrun:
	$(VBOX) createvm --name $(OSNAME) --ostype Other --register
	$(VBOX) modifyvm $(OSNAME) --memory 32
	$(VBOX) storagectl $(OSNAME) --name Floppy --add floppy
	$(VBOX) storageattach $(OSNAME) --storagectl Floppy --device 0 --medium $(IMGNAME)
	$(VBOX) startvm $(OSNAME)
