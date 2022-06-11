	.org $E000

Bank_Flags = $2005
DMA_Flags = $2007
;DMA flags are as follows
; 1   ->   DMA enabled
; 2   ->   Video out page
; 4   ->   NMI enabled
; 8   ->   Colorfill mode enable
; 16  ->   Tile mode disable
; 32  ->   CPU access bank select
; 64  ->   Enable copy completion IRQ
; 128 ->   Transparency copy disable (skips zeroes)

;This program will draw a black stripe across the screen
;I was going to start with drawing a single pixel, but video RAM contents
;are random on boot so a black stripe would be more obvious.


RESET:
	;code here will get run once when the system boots

	;Address $2007, aka the DMA flags, is a bitmask that controls settings of the graphics card
	;This setting will allow the CPU to access video pixels directly
	;It's usually the slowest way to draw, but easier to start with
	;% deontes that a number is written in binary notation
	;# deotes that a number is a constant, instead of a memory address
	LDA #%01100000 ;load this hardcoded value into the A register
	STA DMA_Flags ;store the value from the A register into DMA_Flags
	
	;For now we just initizlize the Bank_Flags register to zero.
	STZ  Bank_Flags


	;In the main loop of this program, register A determines the color of the stripe
	;$ deontes that a number is written in hexadecimal notation
	LDA #$0
	LDX #$0
Forever:
	;code here will run over and over until the system is reset or turned off

	;$5000 corresponds to pixel coordinates (0, 32)
	;Video RAM spans from $4000 to $7FFF
	;Try replacing the $5000 below with a value between $4000 and $7000
	STA $5000, x ;store register A at address $5000+x
	INX ;increment x

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