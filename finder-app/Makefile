CROSS_COMPILE? ?= 
CC = $(CROSS_COMPILE)gcc # runs gcc as default, but allows other compilers to run
CFLAGS = -g -Wall # -g allows gdb usage, -Wall enables warnings

writer: 
	$(CC) $(CFLAGS) writer.c -o writer

clean:
	rm writer
	rm *.o