LIB_XMODULE = libxmodule.a
XMODULE_SUBSRC = ./src

CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
ARFLAGS = -rv
CFLAGS = $(GLB_CFLAGS) -DMAKE_XLIB -I./src -I./include
EXT = c

SRC = $(wildcard $(XMODULE_SUBSRC)/*.$(EXT))
OBJS = $(SRC:.$(EXT)=.o)

all: $(LIB_XMODULE)

.$(EXT).o:
	$(CC) $(CFLAGS) -o $@ -c $<

$(LIB_XMODULE): $(OBJS)
	$(AR) $(ARFLAGS) $(LIB_XMODULE) $(OBJS)

clean:
	rm -rf *.o
	rm -rf *.a
