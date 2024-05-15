CC = gcc
CFLAGS_DEBUG = -g -ggdb -std=c11 -pedantic -W -Wall -Wextra
CFLAGS_RELEASE = -std=c11 -pedantic -W -Wall -Wextra -Werror

.PHONY: clean all
MODE ?= debug
ifeq ($(MODE), debug)
	CFLAGS = $(CFLAGS_DEBUG)
else
	CFLAGS = $(CFLAGS_RELEASE)
endif
all: parent child
parent: parent.c Makefile
	$(CC) $(CFLAGS) parent.c -o parent
child: child.c Makefile
	$(CC) $(CFLAGS) child.c -o child
clean:
	rm -f parent
	rm -f child
	rm -rf parent.dSYM
	rm -rf child.dSYM