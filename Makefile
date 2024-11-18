CC = gcc
CFLAGS = -Wall -g -pthread

# Source files
SRCS = main.c helpers.c log.c
OBJS = $(SRCS:.c=.o)

# Header files 
HEADERS = seating.h structs.h helpers.h log.h

# Target executable
TARGET = dineseating

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile source files to object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
