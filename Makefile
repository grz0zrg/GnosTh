input-path = examples
input-file = logo.th

all: clean program cleanObjs

all-copy: clean program cleanObjs copy

# preprocess Forth sources :
#  1. resolve "; #include file1 file2" (include directives)
#  2. remove comments
#  3. embed Forth code into a .inc as GAS ASM
sample:
	awk '{ if ($$1 == ";") { match($$0, /#include (.*)$$/, arr); if (arr[1]) { split(arr[1], files, " "); for (f in files) { while ((getline line < files[f]) > 0) print line; } next; } } print $$0; }' $(input-path)/${input-file} > program.th.full
	awk -i inplace '{ sub(/;.*/, ""); if ($$0 != "") print }' program.th.full
	awk '{gsub(/"/, "\\\""); print ".ascii \"" $$0 "\\n\""}' program.th.full > program.th.inc
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
	rm -f *.o *.elf program.bin program.th.full program.th.inc

cleanObjs:
	rm -f *.o *.elf