SRCS = $(wildcard *.bmp)

GTGS = $(patsubst %.bmp,%.gtg,$(SRCS))

DEFLATED = $(patsubst %.gtg,%.gtg.deflate,$(GTGS))

all: $(DEFLATED)

%.gtg.deflate: %.gtg
	zopfli --deflate $<

#This makefile assumes the bitmaps to be 128x128 with 8-bits-per-pixel indexed color
#A specific palette is required for the appearance of an image in an art program to match the GameTank output.
%.gtg: %.bmp
	tail -c 16384 $< > $@