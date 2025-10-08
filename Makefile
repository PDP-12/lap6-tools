all: lap6 check

lap6:	lap6.py

check: lap6.py test/test.sh test/test.oct
	test/test.sh
