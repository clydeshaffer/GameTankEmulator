ASM = vasm6502_oldstyle
AFLAGS = -dotdir -wdc02 -Fbin

SRCS = $(wildcard *.asm)

PROGS = $(patsubst %.asm,%.gtr,$(SRCS))

all: $(PROGS) asset_bins

05_audio.gtr: 05_audio.asm asset_bins

asset_bins:
	cd assets && '$(MAKE)'

%.gtr: %.asm

	$(ASM) $(AFLAGS) -o $@ $<