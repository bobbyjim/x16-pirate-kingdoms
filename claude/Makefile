# Host-side build for the simulation engine, CLI, and tests.
# This is deliberately gcc/clang, not cc65 -- per DEV-PROCESS.md, the core
# business logic must be unit-testable off-hardware. A cc65 build of
# engine/ + present/ for the real X16 UI layer is a later phase.

CC      = cc
CFLAGS  = -std=c99 -Wall -Wextra -Iengine -g

ENGINE_SRCS  = engine/rng.c engine/map.c engine/settlement.c engine/events.c engine/world.c engine/note.c engine/storage.c
PRESENT_SRCS = present/present.c
HAL_NULL     = platform/null/gfx_null.c platform/null/sound_null.c
HAL_DEBUG    = platform/debug/gfx_debug.c platform/debug/sound_debug.c
CLI_SRCS     = cli/main.c
TEST_SRCS    = tests/main.c tests/test_settlement.c tests/test_events.c tests/test_map.c tests/test_world.c tests/test_note.c tests/test_storage.c

.PHONY: all cli test clean

all: cli

cli: pk-cli

# The CLI links the debug HAL so `tick` / `event` print a feedback trace;
# swap $(HAL_DEBUG) -> $(HAL_NULL) here for a silent run.
pk-cli: $(ENGINE_SRCS) $(PRESENT_SRCS) $(HAL_DEBUG) $(CLI_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

test: pk-test
	./pk-test

# Tests assert on World.notes directly and never call the presentation
# layer, so they don't link present/ or a HAL at all.
pk-test: $(ENGINE_SRCS) $(TEST_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) pk-cli pk-test
