# Makefile for Coop-tasks Library.

BACKEND = posix

LIBRARY := lib$(NAME)
OBJS := thread_pool.o threads_$(BACKEND).o future.o collections.o
