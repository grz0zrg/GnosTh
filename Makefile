all: clean program cleanObjs

all-copy: clean program cleanObjs copy

# embed Forth source code into a .inc as GAS ASM
sample:
	awk '{gsub(/"/, "\\\""); print ".ascii \"" $$0 "\\n\""}' examples/logo.th > program.th.inc
	awk -i inplace '{gsub(/\t/, "\\t"); print}' program.th.inc

program.o: program.s
	arm-linux-gnueabihf-as -march=armv6 program.s -o program.o

# assemble
program: sample program.o
	arm-linux-gnueabihf-ld -T rpi0.ld program.o -o program.elf
	arm-linux-gnueabihf-objcopy program.elf -O binary program.bin
	wc -c program.bin

# make and disassemble
dump:
	make
	arm-linux-gnueabihf-objdump -D -z -b binary -marm program.bin

# copy binary to SD card and unmount it
copy:
	cp program.bin /media/julien/FCD9-D96C/program.bin
	udisksctl unmount -b /dev/sdg1

# cleanup
clean:
	rm -f *.o *.elf program.bin program.th.inc

cleanObjs:
	rm -f *.o *.elf