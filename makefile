CC	= gcc
ASM64	= yasm -f elf64 -DYASM -D__x86_64__ -DPIC
CFLAGS	= -g -Wall -masm=intel -fno-stack-protector 
PROGS	= libmini.a libmini.so write1 alarm1 alarm2 alarm3 jmp1

all: $(PROGS)

setup:
	ln -s testcase/write1.c write1.c
	ln -s testcase/alarm1.c alarm1.c
	ln -s testcase/alarm2.c alarm2.c
	ln -s testcase/alarm3.c alarm3.c
	ln -s testcase/jmp1.c jmp1.c

shutup:
	rm write1.c alarm1.c alarm2.c alarm3.c jmp1.c

%: %.asm start.o libmini.so
	$(ASM64) $< -o $@.o
	ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o $@ $@.o start.o -L. -L.. -lmini
	rm $@.o

%: %.c start.o libmini.so
	$(CC) -c $(CFLAGS) -nostdlib -I. -I.. -DUSEMINI $<
	ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o $@ $@.o start.o -L. -L.. -lmini
	rm $@.o

start.o: start.asm
	$(ASM64) $< -o $@

libmini.a: libmini64.asm libmini.c
	$(CC) -c $(CFLAGS) -fPIC -nostdlib libmini.c
	$(ASM64) $< -o libmini64.o
	ar rc libmini.a libmini64.o libmini.o

libmini.so: libmini.a
	ld -shared libmini64.o libmini.o -o libmini.so

clean:
	rm -f *.o $(PROGS)
