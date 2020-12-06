	.org $E000

DMA_Flags = $2007
;DMA flags are as follows
; 1   ->   DMA enabled
; 2   ->   Video out page
; 4   ->   NMI enabled
; 8   ->   G.RAM frame select
; 16  ->   V.RAM frame select
; 32  ->   CPU access bank select
; 64  ->   Enable copy completion IRQ
; 128 ->   Transparency copy enabled (skips zeroes)

;This sets up some handy names for memory addresses to make the code clearer
DMA_VX = $4000
DMA_VY = $4001
DMA_GX = $4002
DMA_GY = $4003
DMA_WIDTH = $4004
DMA_HEIGHT = $4005
DMA_Status = $4006
DMA_Color = $4007

;This program will demonstrate the DMA engine by drawing a blue rectangle
;

RESET:
	;code here will get run once when the system boots

	;This setting will give the DMA engine exlusive access to video memory
	;The CPU will access the DMA control parameters instead
	LDA #%01000001
	STA DMA_Flags

	;The DMA engine is controlled by setting each of its parameter registers,
	;then sending the value 1 to DMA_Status.

	;Width and height are set to 16x21 ($10 x $15 in hex)
	;Width and height values can go from 1 to 127
	;(The 8th bit is reserved for a special feature we'll cover later)
	LDA #$10
	STA DMA_WIDTH
	LDA #$15
	STA DMA_HEIGHT

	;Set the top left corner of the drawing area to ($20, $30) or (32, 48)
	LDA #$20
	STA DMA_VX
	LDA #$30
	STA DMA_VY

	;Normally GX,GY are the coordinates in the Graphics RAM to copy from, sort of like a sprite sheet.
	;Here we aren't drawing sprites, and set GX to 128. The 8th bit here tells the engine
	;to draw the color stored in DMA_Color instead of a sprite
	LDA #$80
	STA DMA_GX
	STZ DMA_GY

	;Colors on the GameTank are stored as HHHSSBBB:
	; 3-bit Hue which selects the color of the rainbow to draw
	; 2-bit Saturation indicates how vibrant the color is.
	; 3-bit Brightness for how light/dark the color is
	; When drawing using the DMA_Color register, the number must be inverted to get the right color. 
	LDA #68
	STA DMA_Color

	;Finally store a 1 in DMA_Status to trigger the drawing process.
	;DMA drawing is simultaneous with CPU execution, so you can continue running
	;code while drawing completes
	LDA #1
	STA DMA_Status

	;However, you do need to wait for a DMA draw command to finish before starting another one.
	;The WAI instruction pauses the CPU until any interrupt signal is received.
	;If the 7th bit of DMA_Flags is set, then the DMA engine will send an IRQ interrupt when it finishes.
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