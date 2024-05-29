CC = gcc
CFLAGS_DEBUG = -W -Wall -Wextra -std=c11 -g
CFLAGS_RELEASE = -W -Wall -Wextra -std=c11 -O2
.PHONY: clean all
MODE ?= debug
ifeq ($(MODE), debug)
	CFLAGS = $(CFLAGS_DEBUG)
else
	CFLAGS = $(CFLAGS_RELEASE)
endif
all: main
main: main.c Makefile
	$(CC) $(CFLAGS) main.c -o main
clean:
	rm -f main
	rm -rf main.dSYM