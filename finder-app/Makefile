# define compiler to use
CC = $(CROSS_COMPILE)gcc

# compiler flags
CFLAGS = -Wall -g

# executable name
TARGET = writer

# source files
SOURCES = writer.c
OBJECTS = $(SOURCES:.c=.o)

# default target
all: $(TARGET)

# link object files to create executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

# compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# clean target to remove generated files
clean:
	rm -f $(TARGET) $(OBJECTS)