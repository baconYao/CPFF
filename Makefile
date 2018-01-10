DIR_CON = ./configs

CC = gcc

all: main

main:	
	$(CC) main.c $(DIR_CON)/structure.c -o main.o

#clean
c:
	rm -rf *.o