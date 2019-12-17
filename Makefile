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
all: server

# make the server main file into object
server: server.o
	gcc -o server server.o

# apply C flags to all C files
%.o: %.c Makefile
	$(CC) $(CFLAGS) -fPIC -MMD -MP -c $< -o $@

# clean directory
clean:
	-rm -f server.o server *.d

# clear
.PHONY: all clean
