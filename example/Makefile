# Makefile for Coop-tasks Examples.

EXAMPLES := sample

.PHONY: all clean

all:
	@$(foreach e,$(EXAMPLES),make -f $(ROOTDIR)/Makefile MAKE_OBJS=$(e).mk;)

clean:
	@$(foreach e,$(EXAMPLES),make -f $(ROOTDIR)/Makefile MAKE_OBJS=$(e).mk clean;)
