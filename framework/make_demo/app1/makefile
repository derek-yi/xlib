#!/bin/bash

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
ROOT_PATH = $(shell pwd)/..
TARGET = app_main

#SRC_PATH = common driver service
SRC_PATH = .

CFLAGS = -std=gnu99
CFLAGS += -Wall -Wno-unused -Wno-format
#CFLAGS += -rdynamic -g
CFLAGS += -g

LINK_FLAGS += -lrt -lpthread 

##########################################################
# src and include
##########################################################
#DIRS = $(shell find $(SRC_PATH) -maxdepth 3 -type d)
DIRS = $(SRC_PATH)

SRCS_CPP += $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
SRCS_C += $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
#SRCS_CC += $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))

INC_PATH += -I$(ROOT_PATH)/include 
INC_PATH += $(foreach dir, $(DIRS), -I$(dir))

OBJS_CPP = $(patsubst %.cpp, %.o, $(SRCS_CPP))
OBJS_C = $(patsubst %.c, %.o, $(SRCS_C))

##########################################################
# libs
##########################################################
LIB_PATH += -L$(ROOT_PATH)/lib

LIBS += -llibx1 -llibx2 -lmodx1 -lmodx2

##########################################################
# building
##########################################################
all:$(TARGET)
app:$(TARGET)

$(TARGET) : $(OBJS_CPP) $(OBJS_C)
	@ $(CC) $^ -o $@ $(LINK_FLAGS) $(LIB_PATH) $(LIBS) 
	@ echo Create $(TARGET) ok...

$(OBJS_CPP):%.o : %.cpp
	$(CXX) $(CFLAGS) $< -c -o $@ $(INC_PATH)

$(OBJS_C):%.o : %.c
	$(CC) $(CFLAGS) $< -c -o $@ $(INC_PATH)

.PHONY : clean
clean:
	@ $(RM) $(TARGET) $(OBJS_CPP) $(OBJS_C)
	@ for i in $(MODULES_PATH); \
	do \
	make clean -C $$i; \
	done
	@echo clean all...