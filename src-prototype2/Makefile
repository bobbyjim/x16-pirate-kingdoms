# Makefile for Pirate Kingdoms - X16 Production Build
# Builds for Commander X16 using cl65 toolchain

CC = cl65
AS = ca65
CFLAGS = --cpu 65c02 -t cx16 --create-dep $(<:.c=.d) -Ors -I./core -I./adapters/display -I./adapters/psg
ASFLAGS = --cpu 65c02 -t cx16
LDFLAGS = -t cx16 -m $(PROGRAM).map

PROGRAM = PK
OBJDIR = .obj

# Core game logic (pure C, no hardware deps)
CORE_SRC = core/ship.c core/calendar_engine.c core/map_engine.c

# X16 display adapters (VERA, sprites, conio)
DISPLAY_SRC = adapters/display/sprite.c adapters/display/calendar.c adapters/display/map_display.c

# PSG sound adapter
PSG_SRC = adapters/psg/ADSR.c
PSG_ASM = adapters/psg/adsr1.s

# Main entry point
MAIN_SRC = main.c

# All C sources
SOURCES = $(MAIN_SRC) $(CORE_SRC) $(DISPLAY_SRC) $(PSG_SRC)

# Object files
OBJECTS = $(SOURCES:%.c=$(OBJDIR)/%.o)
ASM_OBJECTS = $(PSG_ASM:%.s=$(OBJDIR)/%.o)

.SUFFIXES:
all: $(PROGRAM)

ifneq ($(MAKECMDGOALS),clean)
-include $(SOURCES:.c=.d)
endif

clean:
	$(RM) -r $(OBJDIR) $(SOURCES:.c=.d) $(PROGRAM) $(PROGRAM).PRG $(PROGRAM).map

$(PROGRAM): $(OBJECTS) $(ASM_OBJECTS)
	$(CC) $(LDFLAGS) -o $@.PRG $^

# Compile C files
$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -o $@ $<

# Assemble ASM files
$(OBJDIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

.PHONY: all clean
