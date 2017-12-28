all: main

main:
	gcc main.c -o main.o

clean:
	rm -f *.o