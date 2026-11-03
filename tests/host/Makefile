# Host unit test (no Zephyr SDK / west needed):
#   make -C tests/host test
CC ?= cc
CFLAGS ?= -Wall -Wextra -Werror -std=c99

REPO_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../..)
SHIM := $(CURDIR)/shim
MOD := $(REPO_ROOT)/modules/openhaystack/src

test: test_openhaystack
	./test_openhaystack

test_openhaystack: test_openhaystack.c $(MOD)/openhaystack.c
	$(CC) $(CFLAGS) -I$(SHIM) -I$(MOD) $^ -o $@

clean:
	rm -f test_openhaystack

.PHONY: test clean
