# Makefile for Coop-tasks Tests.

TESTS := utest

.PHONY: all test clean

all:
	@$(foreach t,$(TESTS),make -f $(ROOTDIR)/Makefile MAKE_OBJS=$(t).mk;)

clean:
	@$(foreach t,$(TESTS),make -f $(ROOTDIR)/Makefile MAKE_OBJS=$(t).mk clean;)

test:
	@./$(UTEST) -r compact $(if $(filter $(V),1),-s --durations yes) $(shell echo "$(TAG)" | grep -o -E -e "\w+" | sed -e "s/\(\w\+\)/[\1]/" | tr -d "\n")
