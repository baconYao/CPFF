#the path of configs
DIR_CON = ./configs

#the path of caching algorithm
DIR_CA = ./modules/prize_caching_manager/prize_caching.c

#the path of caching space policy
DIR_CP = ./modules/caching_space_manager/static_caching_space.c
#DIR_CP = ./modules/caching_space_manager/dynamic_caching_space.c
#DIR_CP = ./modules/caching_space_manager/competition_caching_space.c

DIR_BUG = ./modules/debug/debug.c

CC = gcc

all: main

main:	
	$(CC) main.c $(DIR_CA) $(DIR_CP) $(DIR_BUG) $(DIR_CON)/structure.c -o main.o

#clean
c:
	rm -rf *.o