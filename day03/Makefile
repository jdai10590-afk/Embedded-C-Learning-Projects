CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
TARGET = build/day03_test
SRC = src/main.c src/system.c src/sensor.c src/fault.c

all:
	$(CC) $(SRC) $(CFLAGS) -o $(TARGET)

run:
	./$(TARGET)

clean:
	rm -f $(TARGET)