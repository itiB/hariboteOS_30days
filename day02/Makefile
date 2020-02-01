# ファイル生成規則

ipl.bin : ipl.asm Makefile
	nasm ipl.asm -o ipl.bin -l ipl.lst

# つなげるものをここに
tail.bin : tail.asm Makefile
	nasm tail.asm -o tail.bin -l tail.ipl.lst

# Catでつなげる
helloos.img : ipl.bin tail.bin Makefile
	cat ipl.bin tail.bin > helloos.img

asm :
	make -r ipl.bin

img :
	make -r helloos.img

run :
	make img

clean:
	-rm ipl.bin
	-rm ipl.lst
	-rm tail.bin
	-rm tail.lst

src_only:
	make clean
	rm helloos.img

