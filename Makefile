CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./include
TARGET = lcdcontrol
SRC = src/main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
