# specify compiler
CC = gcc

# specify bin name
BIN_NAME = test_stun

# objects to build
OBJS = network.o unity.o test_stun.o clock.o

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
  -Wno-conversion

# C flags
FLAGS := -g -fPIC -MMD -MP

# libraries
DYNAMIC_LIBS = -lpthread

# make all objects
all: clean $(OBJS)
	$(CC) -o $(BIN_NAME) $(OBJS) $(DYNAMIC_LIBS)

# apply C flags to all C files
%.o: %.c Makefile
	$(CC) $(FLAGS) -c $< -o $@

# clean directory
clean:
	-rm -f main.o tests *.d

# clear
.PHONY: all clean
