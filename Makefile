
all:
	make -C source/boot
	make -C source/loader
	make -C source/kernel

clean:
	make clean -C source/boot
	make clean -C source/loader
	make clean -C source/kernel