CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
DEPFLAGS = -MMD -MP
TARGET = build/day06_test

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c, build/%.o, $(SRCS))
DEPS = $(patsubst src/%.c, build/%.d, $(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

build/%.o: src/%.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f build/*.o build/*.d $(TARGET)

-include $(DEPS)