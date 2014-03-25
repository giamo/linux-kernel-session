all: simple_write test mixed
	
simple_write: simple_write.c
	gcc -Wall -o simple_write simple_write.c

test: test.c
	gcc -Wall -o test test.c -lpthread

mixed: mixed.c
	gcc -Wall -o mixed mixed.c -lpthread
