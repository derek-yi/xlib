#!/bin/bash
############################################################################
#
############################################################################
#源文件，自动找所有.c和.cpp文件，并将目标定义为同名.o文件
SOURCE  := $(wildcard *.c) $(wildcard *.cpp)
OBJS    := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))

TARGET  := libxlib_alg.so
ROOT_DIR := /home/derek/share/xlib
  
############################################################################  
# 编译参数
############################################################################  
# 编译器
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

# 编译宏
DEFINES := -DMAKE_XLIB

# 头文件路径
INCLUDE := -I$(ROOT_DIR)/include  

# 链接库
LIBS := -lpthread 

# 链接选项
LDFLAGS :=

# 汇总处理
CFLAGS := -g -Wall -O3 $(DEFINES) $(INCLUDE) $(LIBS) 
  
############################################################################  
# 下面的基本上不需要做任何改动了
############################################################################  
all:$(OBJS)
	echo $(OBJS)
	$(LD) -shared -o $(TARGET) $(OBJS) $(LDFLAGS)

clean:
	echo "Removing linked and compiled files......"
	rm -fr *.o $(TARGET)

%.o:%.c
	@echo Compiling $< ...
	$(CC) -fPIC -c $(CFLAGS)  $< -o $*.o
