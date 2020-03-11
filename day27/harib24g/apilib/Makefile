OBJS_API = $(patsubst %.asm,%.o,$(shell ls *.asm))

default:
	make apilib.lib
	make -r clean

# ファイルの生成
apilib.lib: Makefile $(OBJS_API)
	ar rcs $@ $(OBJS_API)

# 一般規則
%.o : %.asm Makefile
	nasm -g -f elf $< -o $@

# コマンド
clean:
	rm -f *.o

src_only:
	make -r clean
	rm apilib.lib