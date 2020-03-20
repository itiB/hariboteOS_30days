OSNAME   = helloOS
IMGNAME  = ../haribote.img
VBOX     = /mnt/c/Program\ Files/Oracle/VirtualBox/VBoxManage.exe

default :
	make haribote.img

haribote.img : haribote/ipl10.bin haribote/haribote.sys Makefile \
app/a/a.hrb app/color/color.hrb app/sosu/sosu.hrb app/hello3/hello3.hrb \
app/color2/color2.hrb app/lines/lines.hrb app/walk/walk.hrb \
app/noodle/noodle.hrb

	mformat -f 1440 -C -B haribote/ipl10.bin -i $@ ::
	mcopy -i $@ haribote/haribote.sys ::
	mcopy app/a/a.hrb -i $@ ::
	mcopy app/color/color.hrb -i $@ ::
	mcopy app/hello3/hello3.hrb -i $@ ::
	mcopy app/color2/color2.hrb -i $@ ::
	mcopy app/lines/lines.hrb -i $@ ::
	mcopy app/walk/walk.hrb -i $@ ::
	mcopy app/noodle/noodle.hrb -i $@ ::
	mcopy app/sosu/sosu.hrb -i $@ ::
	mcopy app/sosu3/sosu3.hrb -i $@ ::


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
	make -C app/sosu
	make -C app/sosu3
	make -C app/hello3
	make -C app/color2
	make -C app/lines
	make -C app/walk
	make -C app/noodle

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
	make -r -C app/sosu		clean

src_only_full :
	make -r -C haribote		src_only
	make -r -C apilib		src_only
	make -r -C app/a		src_only
	make -r -C app/color	src_only
	make -r -C app/sosu		src_only
	rm -f haribote.img

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