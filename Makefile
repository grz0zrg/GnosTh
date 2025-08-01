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

# assemble bare program from transpiled LLVM code in 'program.ll' and
# extracted data section in 'program_data.bin' + data loader stub in start.s (target: rpi0 + uboot)
# didn't see any diff. on RPI0 with different -mcpu (chose strongarm due to being close to ARM2)
assemble-llvm:
	opt -O3 program.ll -o program.opt.ll
	llc -march=arm -mcpu=strongarm -O=3 -filetype=obj program.opt.ll -o program.o
	arm-linux-gnueabihf-as --warn --fatal-warnings bare/start.s -o start.o
	arm-linux-gnueabihf-ld -r -b binary program_data.bin -o program_data.o
	arm-linux-gnueabihf-objcopy --add-section .note.GNU-stack=/dev/null program_data.o
	arm-linux-gnueabihf-ld -T bare/rpi0.ld -nostdlib start.o program_data.o program.o -o program.elf
	arm-linux-gnueabihf-objcopy program.elf -O binary program.bin
	wc -c program.bin

# same as above but for tiny binary (target: RPI 0 1.3)
# WARNING: this assume code don't use data (variables, string, array etc.)
#          and no API words, works great for sizecoding graphics stuff !
assemble-llvm-tiny:
	opt -Oz program.ll -o program.opt.ll
	llc -march=arm -mcpu=arm1176jzf-s -O=3 -filetype=obj program.opt.ll -o program.o
	arm-linux-gnueabihf-ld -T rpi0.ld -nostdlib program.o -o program.elf
	arm-linux-gnueabihf-objcopy program.elf -O binary program.bin

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