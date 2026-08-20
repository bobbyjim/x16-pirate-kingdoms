# Host-side build for the simulation engine, CLI, and tests.
# This is deliberately gcc/clang, not cc65 -- per DEV-PROCESS.md, the core
# business logic must be unit-testable off-hardware. A cc65 build of
# engine/ for the real X16 UI layer is a later phase.

CC      = cc
CFLAGS  = -std=c99 -Wall -Wextra -Iengine -g

ENGINE_SRCS = engine/rng.c engine/map.c engine/settlement.c engine/events.c engine/world.c
CLI_SRCS    = cli/main.c
TEST_SRCS   = tests/main.c tests/test_settlement.c tests/test_events.c tests/test_map.c tests/test_world.c

.PHONY: all cli test clean

all: cli

cli: pk-cli

pk-cli: $(ENGINE_SRCS) $(CLI_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

test: pk-test
	./pk-test

pk-test: $(ENGINE_SRCS) $(TEST_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) pk-cli pk-test
