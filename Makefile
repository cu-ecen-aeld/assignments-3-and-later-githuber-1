CROSS_COMPILE? ?= 
CC = $(CROSS_COMPILE)gcc # runs gcc as default, but allows other compilers to run
CFLAGS = -g -Wall # -g allows gdb usage, -Wall enables warnings
WRITER_DIR = ./finder-app

writer: 
	$(CC) $(CFLAGS) $(WRITER_DIR)/writer.c -o $(WRITER_DIR)/writer

clean:
	rm $(WRITER_DIR)/writer
	rm *.o