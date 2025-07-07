# Define variables
CC = gcc
CFLAGS = -Wall -g

# Define the executable name
TARGET = main

# Define the source file
SRC = main.c

# Define the object file
OBJ = $(SRC:.c=.o)

.PHONY: all clean run

# Default target: builds the executable
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

# Rule to compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

# Clean target: removes generated files
clean:
	rm -f $(OBJ) $(TARGET)

run:
	make
	rm -f *.o
	./$(TARGET)