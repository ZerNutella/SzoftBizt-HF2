CC = g++
CFLAGS = -std=c++11 -Wall -Wextra

# Target executable
TARGET = parser

# Source files
SRCS = parser.cpp

# Object files
OBJS = parser.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

$(OBJS): parser.cpp
	$(CC) $(CFLAGS) -c parser.cpp

clean:
	rm -f $(OBJS) $(TARGET)
