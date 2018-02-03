#the path of caching algorithm#
CPFF_PC = cpff_prize_caching.c

#the path of caching space policy#
CPFF_CS = cpff_static_caching_space.c
# CPFF_CS = cpff_dynamic_caching_space.c
# CPFF_CS = cpff_competition_caching_space.c

CPFF_BUG = cpff_debug.c
CPFF_IPC = cpff_ipc.c

CPFF_STR = cpff_structure.c

CC = gcc

all: cpff

cpff: cpff_main.h
	$(CC) cpff_main.c $(CPFF_PC) $(CPFF_CS) $(CPFF_BUG) $(CPFF_IPC) $(CPFF_STR) -o cpff_main

#clean
clean:
	rm -rf cpff_*.o