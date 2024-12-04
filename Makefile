
all:
	make -C source/boot
	make -C source/loader
	make -C source/applib
	make -C source/kernel
	make -C source/app/shell

clean:
	make clean -C source/boot
	make clean -C source/loader
	make clean -C source/applib
	make clean -C source/kernel
	make clean -C source/app/shell