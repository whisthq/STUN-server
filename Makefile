# specify compiler
CC = gcc

# specify bin name
BIN_NAME = server

# objects to build
OBJS = linkedlist.o main.o

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
all: clean $(OBJS)
  $(CC) -o $(BIN_NAME) $(OBJS)

# apply C flags to all C files
%.o: %.c Makefile
  $(CC) $(CFLAGS) -fPIC -MMD -MP -c $< -o $@

# clean directory
clean:
  -rm -f server.o server *.d

# clear
.PHONY: all clean
