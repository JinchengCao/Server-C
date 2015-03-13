all: httpserver

arduino: httpserver.c
	clang -o httpserver httpserver.c

clean: 
	rm -rf *.o

clobber: clean
	rm -rf httpserver