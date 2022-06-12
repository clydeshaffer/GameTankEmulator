Bank_Flags = $2005
DMA_Flags = $2007

;The framebuffer and DMA_VX have the same address, with mapping controled by the lowest bit of DMA_Flags
Framebuffer = $4000
DMA_VX = $4000
DMA_VY = $4001
DMA_GX = $4002
DMA_GY = $4003
DMA_WIDTH = $4004
DMA_HEIGHT = $4005
DMA_Status = $4006
DMA_Color = $4007

inflate_zp = $F0 ; F1, F2, F3

;The binary for INFLATE included in the tutorials must be placed at $E000
	.org $E000
Inflate:
	.incbin "assets/inflate_e000_0200.obx"
;We'll also include the compressed sprite data into the ROM
;See assets/Makefile for how to produce this file
SpriteSheet:
	.incbin "assets/gametank.gtg.deflate"

RESET:
	
	;Init system control registers
	;Specifically we want to directly access Graphics RAM
	;When bit 5 (counting from 0) of DMA_Flags is zero, the VRAM memory range maps to an offscreen
	;sprite buffer instead of a framebuffer.
	;This contains the soure data used by the blitter in drawing operations.
	LDA #1
	STA DMA_Flags
	STZ Bank_Flags

	;GX and GY counters affect the GRAM mapping so we zero
	;them and activate a 1x1 blit to transfer values from
	;registers to counters
	STZ DMA_GX
	STZ DMA_GY
	STA DMA_WIDTH
	STA DMA_HEIGHT
	STA DMA_Status
	STZ DMA_Status
	STZ DMA_Flags


	;In this example we're loading compressed graphics into GRAM with INFLATE
	;To set this up we put the address of the compressed data at inflate_zp and inflate_zp+1
	;and the destination address at inflate_zp+2 and inflate_zp+3.
	;Then we just jump to the included binary as a subroutine.
	LDA #<SpriteSheet
	STA inflate_zp
	LDA #>SpriteSheet
	STA inflate_zp+1
	LDA #<Framebuffer
	STA inflate_zp+2
	LDA #>Framebuffer
	STA inflate_zp+3
	JSR Inflate
	;This doesn't take all that long, but definitely more than one frame.

	;Next we'll set up the DMA_Flags register for drawing a sprite with a transparent background.
	;Bit 0 enables DMA operations
	;Bit 4 disables tiling mode. Rather, it enables the carry on the source coordinate counter.
	;When bit 4 is not set, a blit will repeat a 16x16 tile instead of drawing the full source image
	LDA #%00010001
	STA DMA_Flags

	;Let's draw (almost) this whole image with the blitter.
	;The max size for a blit operation is 127x127
	LDA #$7F
	STA DMA_WIDTH
	STA DMA_HEIGHT

	;Drawing this with its upper-left corner at (0,0)
	STZ DMA_VX
	STZ DMA_VY

	;We'll also start drawing from (0,0) in the source data
	STZ DMA_GX
	STZ DMA_GY

	;Begin the draw by writing 1 to DMA_Status
	LDA #1
	STA DMA_Status

	;Wait for it to finish
	WAI
Forever:
	JMP Forever

IRQ:
	;code here will run after certain interrupts (if enabled)
	;such as when a DMA draw call completes
	RTI
NMI:
	;code here will run once per frame (if enabled)
	RTI

	.org $FFFA
	.dw NMI
	.dw RESET
	.dw IRQ