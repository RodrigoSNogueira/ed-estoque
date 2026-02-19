# Makefile - Sistema de Estoque (C)

CC      := clang
CFLAGS  := -Wall -Wextra -O2
TARGET  := estoque
SRC     := sistema-estoque.c

.PHONY: all run clean rebuild

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

rebuild: clean all
