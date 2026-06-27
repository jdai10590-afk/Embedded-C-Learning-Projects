CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
TARGET = build/day04_test

OBJS = build/main.o build/system.o build/sensor.o build/fault.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

build/main.o: src/main.c
	$(CC) $(CFLAGS) -c src/main.c -o build/main.o

build/system.o: src/system.c
	$(CC) $(CFLAGS) -c src/system.c -o build/system.o

build/sensor.o: src/sensor.c
	$(CC) $(CFLAGS) -c src/sensor.c -o build/sensor.o

build/fault.o: src/fault.c
	$(CC) $(CFLAGS) -c src/fault.c -o build/fault.o

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f build/*.o $(TARGET)