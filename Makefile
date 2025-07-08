# Define variables
CC = gcc
CFLAGS = -Wall -Wextra -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lgd -lpng -lz -ljpeg -lfreetype -lm

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

# Rule to compile source files into object files
$(OBJ): $(SRC)

# Clean target: removes generated files
clean:
	rm -f $(OBJ) $(TARGET)

run:
	make
	rm -f *.o
	./$(TARGET)