all: main

main:
	gcc -c main.c -o main.o
clean:
	rm -f main.o