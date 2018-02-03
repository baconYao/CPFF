#the path of configs#
DIR_CON = cpff_configs

#the path of caching algorithm#
DIR_CA = cpff_prize_caching.c

#the path of caching space policy#
DIR_CP = cpff_static_caching_space.c
#DIR_CP = cpff_dynamic_caching_space.c
#DIR_CP = cpff_competition_caching_space.c

DIR_BUG = cpff_debug.c
DIR_IPC = cpff_ipc.c

CC = gcc

all: cpff

cpff:	$(CPFF_OBJ)
	$(CC) cpff_main.c $(DIR_CA) $(DIR_CP) $(DIR_BUG) $(DIR_IPC) cpff_structure.c -o cpff_main.o

#clean
clean:
	rm -rf *.o