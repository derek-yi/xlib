SOURCE = $(wildcard *.c)  
TARGETS = $(patsubst %.c, %, $(SOURCE))  
BIN_FILES = $(patsubst %.c, %.bin, $(SOURCE)) 

CC = gcc  
CFLAGS = -Wall -g -lpthread -lrt

all:$(TARGETS)  

$(TARGETS):%:%.c  
	$(CC) $< $(CFLAGS) -o $@.bin

.PHONY:clean all  
clean:  
	-rm -rf $(BIN_FILES)

