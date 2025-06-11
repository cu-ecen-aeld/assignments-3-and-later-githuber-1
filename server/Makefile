CROSS_COMPILE? ?= # defaults to empty string
CC = $(CROSS_COMPILE)gcc # runs gcc as default, but allows other compilers to run
CFLAGS = -g -Wall -Werror# -g allows gdb usage, -Wall enables warnings

all: 
	$(CC) $(CFLAGS) aesdsocket.c -o aesdsocket

clean:
	rm -f aesdsocket
	rm -f *.o