obj-m := hellofs.o
hellofs-objs := khellofs.o super.o inode.o dir.o file.o
CFLAGS_khellofs.o := -DDEBUG
CFLAGS_super.o := -DDEBUG
CFLAGS_inode.o := -DDEBUG
CFLAGS_dir.o := -DDEBUG
CFLAGS_file.o := -DDEBUG
CFLAGS_mkfs-hellofs.o := -Wall -ansi -std=c99
CFLAGS_dumpfs-hellofs.o := -Wall -ansi -std=c99

all: ko mkfs-hellofs dmpfs-hellofs

ko:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

mkfs-hellofs_SOURCES:
	mkfs-hellofs.c hellofs.h

dmpfs-hellofs.o: dmpfs-hellofs.c hellofs.h Makefile
	gcc -c -Wall -ansi -std=c99 -g $<

dmpfs-hellofs: dmpfs-hellofs.o Makefile
	gcc -o $@ -g $<

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f mkfs-hellofs.o mkfs-hellofs dmpfs-hellofs.o dmpfs-hellofs
