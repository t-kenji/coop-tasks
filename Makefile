# Makefile for Coop-tasks Library.

ifeq ($(MAKELEVEL),0)

export ROOTDIR := $(PWD)

include $(ROOTDIR)/config.mk

.PHONY: all build-all \
        build build-test build-example \
        test \
        example \
        doc \
        cppcheck scan-build \
        install install-test install-example \
        clean

all: build-all

build-all: build build-test

build:
	@make -f $(ROOTDIR)/Makefile MAKE_OBJS=objs.mk -C src

build-test: build
	@make -C test

build-example: build
	@make -C example

test: build-test
	@make -C test test

doc:
	@sed -e 's|@PROJECT@|$(DOXY_PROJECT)|' \
	     -e 's|@VERSION@|$(DOXY_VERSION)|' \
	     -e 's|@BRIEF@|$(DOXY_BRIEF)|' \
	     -e 's|@OUTPUT@|$(DOXY_OUTPUT)|' \
	     -e 's|@SOURCES@|$(DOXY_SOURCES)|' \
	     -e 's|@EXCLUDE@|$(DOXY_EXCLUDE)|' \
	     -e 's|@EXCLUDE_SYMBOLS@|$(DOXY_EXCLUDE_SYMBOLS)|' \
	     -e 's|@PREDEFINED@|$(DOXY_PREDEFINED)|' \
	     -e 's|@README@|README.md|' \
	     -e 's|@EXTRACT_STATIC@|YES|' \
	     -e 's|@SHOW_FILES@|YES|' \
	     -e 's|@SOURCE_BROWSER@|YES|' \
	     Doxyfile.in > Doxyfile
	@doxygen Doxyfile


cppcheck:
	@make -C src cppcheck

scan-build:
	@scan-build make build

install: install-test install-example

install-test: build-test
	@install -d $(DESTDIR)/bin
	install -m 0755 ./test/$(UTEST) $(DESTDIR)/bin

install-example: build-example
	@install -d $(DESTDIR)/bin
	install -m 0755 ./example/sample $(DESTDIR)/bin

clean:
	@rm -rf Doxyfile $(DOXY_OUTPUT)
	@make -f $(ROOTDIR)/Makefile MAKE_OBJS=objs.mk -C src clean
	@make -C example clean
	@make -C test clean

else
include $(MAKE_OBJS)
include $(ROOTDIR)/rules.mk
endif
