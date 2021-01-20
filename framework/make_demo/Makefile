
##########################################################
# tools
##########################################################
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++

MV = mv -f
RM = rm -rf
LN = ln -sf

##########################################################
# path and flags
##########################################################
TOP_PATH = $(shell pwd)
TARGET = my_app

## source code
SRC_PATH = src1 src2

## cc flags
CFLAGS = -std=gnu99
CFLAGS += -Wall -Wno-unused -Wno-format
#CFLAGS += -g

## link flags
LINK_FLAGS += -lrt -lpthread -lzlog -lm

# external lib  
LIB_PATH += -L$(TOP_PATH)/lib
LIBS += 

##########################################################
# src and include
##########################################################
#DIRS = $(shell find $(SRC_PATH) -maxdepth 3 -type d)
DIRS = $(SRC_PATH)

SRCS_CPP += $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
SRCS_C += $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))

INC_PATH += -I$(TOP_PATH)/include
INC_PATH += $(foreach dir, $(SRC_PATH), -I$(dir))

OBJS_CPP = $(patsubst %.cpp, %.o, $(SRCS_CPP))
OBJS_C = $(patsubst %.c, %.o, $(SRCS_C))

##########################################################
# modules
##########################################################
MOD_PATH = $(TOP_PATH)/lib
modules = libx1 libx2
LINK_FLAGS += -lx1 -lx2

MODULES_PATH = $(foreach m, $(modules), $(MOD_PATH)/$(m))

##########################################################
# building
##########################################################
all:$(TARGET)

$(TARGET) : $(OBJS_CPP) $(OBJS_C)
	@ $(CC) $^ -o $@ $(LIB_PATH) $(LIBS) $(LINK_FLAGS)
	@ echo Create $(TARGET) ok...

$(OBJS_CPP):%.o : %.cpp
	$(CXX) $(CFLAGS) $< -c -o $@ $(INC_PATH)

$(OBJS_C):%.o : %.c
	$(CC) $(CFLAGS) $< -c -o $@ $(INC_PATH)

modules:
	@ for i in $(MODULES_PATH); \
	do \
	make -C $$i; \
	done

.PHONY : clean
clean:
	@ $(RM) $(TARGET) $(OBJS_CPP) $(OBJS_C)
	@ for i in $(MODULES_PATH); \
	do \
	make clean -C $$i; \
	done
	@echo clean all...

	