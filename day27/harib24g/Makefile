OSNAME   = helloOS
IMGNAME  = ../haribote.img
VBOX     = /mnt/c/Program\ Files/Oracle/VirtualBox/VBoxManage.exe

default :
	make haribote.img

haribote.img : haribote/ipl10.bin haribote/haribote.sys Makefile \
app/a/a.hrb app/color/color.hrb
	mformat -f 1440 -C -B haribote/ipl10.bin -i $@ ::
	mcopy -i $@ haribote/haribote.sys ::
	mcopy app/a/a.hrb -i $@ ::
	mcopy app/color/color.hrb -i $@ ::

run :
	make haribote.img
	mv haribote.img ../
	make vdel
	make vrun

full :
	make -C haribote
	make -C apilib
	make -C app/a
	make -C app/color
	make haribote.img

run_full :
	make -r full
	mv haribote.img ../
	make vdel
	make vrun

run_os :
	make -C haribote
	make -r run

clean :
	# 特になし

src_only :
	make clean
	rm -f haribote.img

clean_full :
	make -r -C haribote		clean
	make -r -C apilib		clean
	make -r -C app/a		clean
	make -r -C app/color	clean

src_only_full :
	make -r -C haribote		src_only
	make -r -C apilib		src_only
	make -r -C app/a		src_only
	make -r -C app/color	src_only

reflesh :
	make full
	make clean_full
	rm -f haribote.img

# VirtualBox
vdel:
	$(VBOX) unregistervm $(OSNAME) --delete

vrun:
	$(VBOX) createvm --name $(OSNAME) --ostype Other --register
	$(VBOX) modifyvm $(OSNAME) --memory 32
	$(VBOX) storagectl $(OSNAME) --name Floppy --add floppy
	$(VBOX) storageattach $(OSNAME) --storagectl Floppy --device 0 --medium $(IMGNAME)
	$(VBOX) startvm $(OSNAME)