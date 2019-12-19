# specify compiler
CC = gcc

# warnings
WARNINGS = \
	-Wall \
	-Wextra \
	-Wshadow \
	-Wpointer-arith \
	 -Wcast-align \
	 -Wwrite-strings \
	 -Wmissing-declarations \
	 -Wredundant-decls \
	 -Winline \
	 -Wno-long-long \
	 -Wuninitialized \
	 -Wconversion

# C warnings
CWARNINGS := $(WARNINGS) \
	-Wmissing-prototypes \
	-Wnested-externs \
	-Wstrict-prototypes

# C flags
CFLAGS := -g -fPIC -std=c99 $(CWARNINGS)

# make all objects
all: linkedlist server

# make the linked list file into object
linkedlist: linkedlist.o
	gcc -c linkedlist.c linkedlist.o

# make the server main file into object
server: server.o
	gcc -o server.c server.o

# apply C flags to all C files
%.o: %.c Makefile
	$(CC) $(CFLAGS) -fPIC -MMD -MP -c $< -o $@

# clean directory
clean:
	-rm -f server.o server *.d

# clear
.PHONY: all clean
