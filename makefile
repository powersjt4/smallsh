###
###@file - Make file for CS162
###@author - Jacob Powers
###@description - Make file to compile projects
### includes testc: which is simple g++ compile all
### testo: is complie individual changes
### debug: includes -g flag for GDB
### clean: removes all executables and .o 

CC = gcc
#CFLAGS = -std=c99
CFLAGS = -std=gnu99
CFLAGS += -Wall 
CFLAGS += -pedantic-errors
LDFLAGS = -lm
OBJS = smallsh.o
SRCS = smallsh.c
HEADERS = 

default: all 

 all: 
	$(CC) ${CFLAGS} ${SRCS} -o smallsh $(LDFLAGS)

test: smallsh.c
	${CC} ${CFLAGS} -g -v smallsh.c -o test $(LDFLAGS)

clean:
	${RM} -rf smallsh test  *.o i in out junk junk2 infile outfile test.dSYM
