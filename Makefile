CFLAGS=-g -Wall

all: lap6.py convert lap6 check

convert: convert.o
	$(CC) $(CFLAGS) $^ -o $@

lap6: lap6.o
	$(CC) $(CFLAGS) $^ -o $@

check: lap6.py test/test.sh test/test.oct
	test/test.sh
