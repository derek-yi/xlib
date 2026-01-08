# Top level makefile

## dynamic lib
DYN_LIB_DIR=libx1

## static lib
ST_LIB_DIR=src1

## app list
APP_DIR=app1

app:
	for var in $(APP_DIR); do \
        cd $$var && $(MAKE) $@; \
		cd -; \
    done

dlibs:
	for var in $(DYN_LIB_DIR); do \
        cd $$var && $(MAKE) dynamic; \
		cd -; \
    done

slibs:
	for var in $(ST_LIB_DIR); do \
        cd $$var && $(MAKE) static; \
		cd -; \
    done

clean:
	for var in $(DYN_LIB_DIR); do \
		cd $$var && $(MAKE) $@; \
		cd -; \
	done
	for var in $(ST_LIB_DIR); do \
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
