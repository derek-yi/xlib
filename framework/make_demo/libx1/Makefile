# copy from zlog makefile
# Copyright (C) 2010-2012 Hardy Simpson <HardySimpson1984@gmail.com>
# This file is released under the LGPL 2.1 license, see the COPYING file

OBJ= libx1.o

LIBNAME=liblibx1
LIB_OUTDIR=../lib

VER_MAJOR=0
VER_MINOR=1

CFLAGS += -DMAKE_XLIB -I../include
########################################################################################
# Fallback to gcc when $CC is not in $PATH.
CC:=$(shell sh -c 'type $(CC) >/dev/null 2>/dev/null && echo $(CC) || echo gcc')
OPTIMIZATION?=
WARNINGS=-Wall -Wstrict-prototypes -fwrapv
DEBUG?= -g -ggdb
REAL_CFLAGS=$(OPTIMIZATION) -fPIC -pthread $(CFLAGS) $(WARNINGS) $(DEBUG)
REAL_LDFLAGS=$(LDFLAGS) -pthread -lrt

DYLIBSUFFIX=so
STLIBSUFFIX=a
DYLIB_MINOR_NAME=$(LIBNAME).$(DYLIBSUFFIX).$(VER_MAJOR).$(VER_MINOR)
DYLIB_MAJOR_NAME=$(LIBNAME).$(DYLIBSUFFIX).$(VER_MAJOR)
DYLIBNAME=$(LIBNAME).$(DYLIBSUFFIX)
DYLIB_MAKE_CMD=$(CC) -shared -Wl,-soname,$(DYLIB_MINOR_NAME) -o $(DYLIBNAME) $(LDFLAGS)
STLIBNAME=$(LIBNAME).$(STLIBSUFFIX)
STLIB_MAKE_CMD=ar rcs $(STLIBNAME)

# Platform-specific overrides
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
compiler_platform := $(shell sh -c '$(CC) --version|grep -i apple')

ifeq ($(uname_S),SunOS)
#  REAL_LDFLAGS+= -ldl -lnsl -lsocket
  DYLIB_MAKE_CMD=$(CC) -G -o $(DYLIBNAME) -h $(DYLIB_MINOR_NAME) $(LDFLAGS)
  INSTALL= cp -r
endif

# For Darwin builds, check the compiler platform above is not empty. The covers cross compilation on Linux
ifneq ($(compiler_platform),)
  DYLIBSUFFIX=dylib
  DYLIB_MINOR_NAME=$(LIBNAME).$(VER_MAJOR).$(VER_MINOR).$(DYLIBSUFFIX)
  DYLIB_MAJOR_NAME=$(LIBNAME).$(VER_MAJOR).$(DYLIBSUFFIX)
  DYLIB_MAKE_CMD=$(CC) -dynamiclib -install_name $(INSTALL_LIBRARY_PATH)/$(DYLIB_MINOR_NAME) -o $(DYLIBNAME) $(LDFLAGS)
endif

ifeq ($(uname_S),AIX)
  # this logic of minor major is not relevant on AIX or at least not widely used
  # not to mention dynamic linker .a preference...
  DYLIB_MAKE_CMD=$(CC) -shared -Wl,-G,-b64 -maix64 -pthread -o $(DYLIBNAME) $(LDFLAGS)
  REAL_CFLAGS+= -maix64
  STLIB_MAKE_CMD=OBJECT_MODE=64 ar rcs $(STLIBNAME) $(DYLIB_MAJOR_NAME)
endif

all: $(DYLIBNAME) 
########################################################################################
# Deps (use make dep to generate this)
libx1.o: libx1.c ../include/pub1.h libx1.h






########################################################################################
$(DYLIBNAME): $(OBJ)
	$(DYLIB_MAKE_CMD) $(OBJ) $(REAL_LDFLAGS)
	# for use in test folder - linux and requirement for aix runtime
	# resolving
	cp -f $(DYLIBNAME) $(DYLIB_MAJOR_NAME)
	cp -f $(DYLIBNAME) $(DYLIB_MINOR_NAME)
	cp -f $(DYLIBNAME) $(LIB_OUTDIR)

$(STLIBNAME): $(OBJ)
	$(STLIB_MAKE_CMD) $(OBJ)
	cp -f $(STLIBNAME) $(LIB_OUTDIR)

dynamic: $(DYLIBNAME)
static: $(STLIBNAME)

.c.o:
	$(CC) -std=gnu99 -pedantic -c $(REAL_CFLAGS) $<

clean:
	rm -rf $(DYLIBNAME) $(STLIBNAME) *.o $(DYLIB_MINOR_NAME) $(DYLIB_MAJOR_NAME)

dep:
	$(CC) -MM -I../include *.c

32bit:
	@echo ""
	@echo "WARNING: if this fails under Linux you probably need to install libc6-dev-i386"
	@echo ""
	$(MAKE) CFLAGS="-m32" LDFLAGS="-m32"

noopt:
	$(MAKE) OPTIMIZATION=""

.PHONY: all clean dep 32bit noopt 
########################################################################################
