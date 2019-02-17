# Configuration for Coop-tasks Library.
export

# Project options.
NAME := tasks
MAJOR_VERSION := 0
MINOR_VERSION := 0
REVISION := 1
VERSION := $(MAJOR_VERSION).$(MINOR_VERSION).$(REVISION)
DESTDIR ?=

# Build target.
#  native: Build for native.
PLATFORM ?= native
ENABLE_STATIC := 1
ENABLE_SHARED := 0

# Build type.
#  release: No debuggable.
#  debug: Debuggable.
BUILD_TYPE := debug

# Doxygen options.
DOXY_PROJECT := "Cooperative Task System Reference Manual"
DOXY_BRIEF := "Cooperative Task System"
DOXY_VERSION := "0.01"
DOXY_OUTPUT := doxygen
DOXY_SOURCES := include src

# Dependencies.
CATCH2_DIR ?=

# Test options.
UTEST := $(NAME)_utest
INTERNAL_TESTABLE ?= 1

# Debug options.
NODEBUG ?= 0
WARN_AS_ERROR ?= 0
ENABLE_SANITIZER ?= 1
DISABLE_CCACHE ?= 0

# Compile & Link options.
CSTANDARD ?= c11
CXXSTANDARD ?= c++11
EXTRA_CPPFLAGS ?=
EXTRA_CFLAGS ?=
EXTRA_CXXFLAGS ?=
EXTRA_LDFLAGS ?=
EXTRA_INCS ?= -I$(ROOTDIR)/include
EXTRA_LDLIBS ?= -lpthread -lrt -latomic

# Verbose options.
V ?= 0
