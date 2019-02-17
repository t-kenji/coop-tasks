# Makefile for Coop-tasks Tests.

CONFIG_TEST_COLLECTIONS := y
CONFIG_TEST_THREADS := y
CONFIG_TEST_THREAD_POOL := y

test-$(CONFIG_TEST_COLLECTIONS) += collections.o
test-$(CONFIG_TEST_THREADS) += threads.o
test-$(CONFIG_TEST_THREAD_POOL) += thread_pool.o

TEST = $(UTEST)
OBJS = main.o utils.o $(test-y)
EXTRA_CXXFLAGS += -I$(ROOTDIR)/src
EXTRA_CXXFLAGS += $(if $(CATCH2_DIR),-I$(CATCH2_DIR)/single_include)
