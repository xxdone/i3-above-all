CC = gcc
SRCS = main.c
CFLAGS = -lX11
TARGET = above_all
PREFIX := /usr/local/bin

all:
	$(CC) $(SRCS) $(CFLAGS) -o $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) $(PREFIX)

remove:
	rm -f $(PREFIX)/$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all install remove clean
