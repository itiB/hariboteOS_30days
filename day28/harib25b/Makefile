OSNAME  = helloOS
IMGNAME = ../haribote.img
VBOX    = /mnt/c/Program\ Files/Oracle/VirtualBox/VBoxManage.exe
APPDIR  = app
APPS   := $(shell ls -d $(APPDIR)/*/)

define \n


endef

echo :
	echo $(APPS)

default :
	make haribote.img

haribote.img : haribote/ipl10.bin haribote/haribote.sys Makefile $(APPS)/*.hrb
	mformat -f 1440 -C -B haribote/ipl10.bin -i $@ ::
	mcopy -i $@ haribote/haribote.sys ::
	$(foreach FILE,$(APPS),mcopy $(FILE)*.hrb -i $@ ::$(\n))


run :
	make haribote.img
	mv haribote.img ../
	make vdel
	make vrun

full :
	make -C haribote
	make -C apilib
	$(foreach FILE,$(APPS),make -C $(FILE)$(\n))
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
	$(foreach FILE,$(APPS),make -r -C $(FILE) clean$(\n))

src_only_full :
	make -r -C haribote		src_only
	make -r -C apilib		src_only
	$(foreach FILE,$(APPS),make -r -C $(FILE) src_only$(\n))
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