CFLAGS=-g -Wall -Wextra

all: lap6.py convert lap6 tabify check

convert: convert.o
	$(CC) $(CFLAGS) $^ -o $@

lap6: lap6.o disassemble.o
	$(CC) $(CFLAGS) $^ -o $@

tabify: tabify.o
	$(CC) $(CFLAGS) $^ -o $@

check: lap6.py test/test.sh test/assembler.oct
	test/test.sh
