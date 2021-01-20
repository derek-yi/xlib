#####################################################
#
#####################################################
#target you can change test to what you want
#run-time library name，lib*.so
TARGET  := libx1.so

TOP_PATH = $(shell pwd)/../..
#####################################################
# compile and lib parameter
#####################################################
CC := $(CROSS_COMPILE)gcc
CP := cp -f

DEFINES :=
INCLUDE := -I. -I$(TOP_PATH)/include
CFLAGS  := -g -Wall -O3 $(DEFINES) $(INCLUDE)
CXXFLAGS:= $(CFLAGS) -DHAVE_CONFIG_H

SHARE   := -fPIC -shared -o
LIBS    :=
LDFLAGS :=
#####################################################
#
#####################################################
#source file
SOURCE  := $(wildcard *.c) $(wildcard *.cpp)
OBJS    := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))

.PHONY : everything objs clean veryclean rebuild

everything : $(TARGET)

all : $(TARGET)

objs : $(OBJS)

rebuild: veryclean everything

clean :
	rm -fr *.o
	rm -fr $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CXXFLAGS) $(SHARE) $@ $(OBJS) $(LDFLAGS) $(LIBS)
	$(CP) $(TARGET) $(TOP_PATH)/lib