#the path of configs
DIR_CON = ./configs

#the path of modules
DIR_MOD = ./modules

CC = gcc

all: main

main:	
	$(CC) main.c $(DIR_MOD)/prize_caching_manager/prize_caching.c $(DIR_CON)/structure.c -o main.o

#clean
c:
	rm -rf *.o