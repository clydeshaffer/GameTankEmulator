ASM = vasm6502_oldstyle
AFLAGS = -dotdir -wdc02 -Fbin

SRCS = $(wildcard *.asm)

PROGS = $(patsubst %.asm,%.gtr,$(SRCS))

all: $(PROGS)

%.gtr: %.asm

	$(ASM) $(AFLAGS) -o $@ $<