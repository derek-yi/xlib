# Top level makefile

## dynamic lib
DLIB_DIR=libx1 libx2

## static lib
SLIB_DIR=src1 src2

## app list
APP_DIR=app1

app:
	for var in $(APP_DIR); do \
        cd $$var && $(MAKE) $@; \
		cd -; \
    done

dlibs:
	for var in $(DLIB_DIR); do \
        cd $$var && $(MAKE) dynamic; \
		cd -; \
    done

slibs:
	for var in $(SLIB_DIR); do \
        cd $$var && $(MAKE) static; \
		cd -; \
    done

clean:
	for var in $(DLIB_DIR); do \
		cd $$var && $(MAKE) $@; \
		cd -; \
	done
	for var in $(SLIB_DIR); do \
		cd $$var && $(MAKE) $@; \
		cd -; \
	done 
	for var in $(APP_DIR); do \
		cd $$var && $(MAKE) $@; \
		cd -; \
	done 

distclean: clean

dummy:

.PHONY: all
all: dlibs slibs app
