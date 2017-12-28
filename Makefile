all: main

main: main.o
	gcc -c main.c -o main.o
clean:
	rm -f main.o