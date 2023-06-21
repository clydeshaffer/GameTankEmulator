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

VIA = $2800
ORB = 0
ORA = 1
DDRB = 2
DDRA = 3
T1C = 5
ACR = $B
PCR = $C
IFR = $D
IER = $E

inflate_zp = $F0 ; F1, F2, F3

;We can store much more than one tilemap in a page, so we'll track the current map in the page with a pointer variable
current_tilemap = $C0 ; and $C1
current_page_num = $01

temp = $00 ; an extra temp variable always comes in handy

;Sections should be 16K bytes long, so we'll pad it out a bit here
    .org $C000
    .byte 0

;The binary for INFLATE included in the tutorials must be placed at $E000
	.org $E000
Inflate:
	.incbin "assets/inflate_e000_0200.obx"
;We'll also include the compressed sprite data into the ROM
;See assets/Makefile for how to produce this file
SpriteSheet:
	.incbin "assets/cubicle.gtg.deflate"

ClearScreenBlack:
	LDA #$7F
	STA DMA_WIDTH
	STA DMA_HEIGHT
	STZ DMA_VX
	STZ DMA_VY
	STZ DMA_GX
	STZ DMA_GY
	LDA #$FE
	STA DMA_Color
	LDA #1
	STA DMA_Status
	WAI
	RTS

;Draw an 8x8 tilemap starting from the current_tilemap pointer
;Skip drawing tiles that equal zero
DrawTilemap:
	LDA #$08
	STA DMA_WIDTH
	LDA #$08
	STA DMA_HEIGHT
	LDY #$0
TilemapLoop:
	LDA (current_tilemap), y
	CMP #$EF
	BCS SkipTile
	TYA
	AND #$0F
	ASL
	ASL
	ASL
	STA DMA_VX
	TYA
	AND #$F0
	LSR
	STA DMA_VY
	LDA (current_tilemap), y
	AND #$0F
	ASL
	ASL
	ASL
	STA DMA_GX
	LDA (current_tilemap), y
	AND #$F0
	LSR
	STA DMA_GY
	LDA #1
	STA DMA_Status
	WAI
SkipTile:
	INY
	BNE TilemapLoop
	RTS

;Our page change routine
ShiftHighBits:
    STA temp
    LDA #$FF
    STA VIA+ORA


    LDA temp
    ROR
    ROR
    ROR
    ROR
    ROR
    AND #2
    ORA #%00000100
    STA VIA+ORA
    ORA #1
    STA VIA+ORA


    LDA temp
    ROR
    ROR
    ROR
    ROR
    AND #2
    ORA #%00000100
    STA VIA+ORA
    ORA #1
    STA VIA+ORA


    LDA temp
    ROR
    ROR
    ROR
    AND #2
    ORA #%00000100
    STA VIA+ORA
    ORA #1
    STA VIA+ORA


    LDA temp
    ROR
    ROR
    AND #2
    ORA #%00000100
    STA VIA+ORA
    ORA #1
    STA VIA+ORA
   
    LDA temp
    ROR
    AND #2
    ORA #%00000100
    STA VIA+ORA
    ORA #1
    STA VIA+ORA


    LDA temp
    AND #2
    ORA #%00000100
    STA VIA+ORA
    ORA #1
    STA VIA+ORA


    LDA temp
    ROL
    AND #2
    STA VIA+ORA
    ORA #1
    STA VIA+ORA


    ORA #4
    STA VIA+ORA


    RTS

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

    ;Initialize the VIA
    LDA #%00000111
    STA VIA+DDRA
    LDA #$FF
    STA VIA+ORA

    LDA #$81
    JSR ShiftHighBits

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

    ;Initialize current tilemap pointer to the beginning of a ROM page, $8000
    LDA #$00
    STA current_tilemap
    LDA #$80
    STA current_tilemap+1

    LDA #%11011101
    STA DMA_Flags

    LDA #$80
    STA current_page_num
Forever:
    WAI
    LDA #%11011101
    STA DMA_Flags
    JSR ClearScreenBlack

    ;Next we'll set up the DMA_Flags register for drawing a sprite with a transparent background.
	;Bit 0 enables DMA operations
	;Bit 4 disables tiling mode. Rather, it enables the carry on the source coordinate counter.
	;When bit 4 is not set, a blit will repeat a 16x16 tile instead of drawing the full source image
	LDA #%01010101
	STA DMA_Flags

	;Call the tilemap drawing subroutine
    JSR DrawTilemap

    LDA current_page_num
    EOR #1
    STA current_page_num
    JSR ShiftHighBits

    LDX #0
WaitLoop:
    WAI
    INX
    BNE WaitLoop

	JMP Forever


IRQ:
	;code here will run after certain interrupts (if enabled)
	;such as when a DMA draw call completes
    STZ DMA_Status
	RTI
NMI:
	;code here will run once per frame (if enabled)
	RTI

	.org $FFFA
	.dw NMI
	.dw RESET
	.dw IRQ