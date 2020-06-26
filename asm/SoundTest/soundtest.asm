tmp = $0

ArrowIndex = $30

FrameFlag = $34
FrameFlip = $35

GamePad1BufferA = $36
GamePad1BufferB = $37
;GamePad2BufferA = $38
;GamePad2BufferB = $39
Old_GamePad1BufferA = $3A
Old_GamePad1BufferB = $3B
;Old_GamePad2BufferA = $3C
;Old_GamePad2BufferB = $3D

PrintNum_X = $40
PrintNum_Y = PrintNum_X+1
PrintNum_N = PrintNum_X+2

var_SquareNote1 = $50
var_SquareCtrl1 = $51
var_SquareNote2 = $52
var_SquareCtrl2 = $53
var_NoiseCtrl   = $54
var_reserved	= $55 ;controls nothing but makes the menu easier to implement
var_WaveNote    = $56
var_WaveCtrl    = $57

inflate_zp = $F0

SquareNote1 = $2000
SquareCtrl1 = $2001
SquareNote2 = $2002
SquareCtrl2 = $2003
NoiseCtrl = $2004
WaveNote = $2005
WaveCtrl = $2006

DMA_Flags = $2007

GamePad1 = $2008
GamePad2 = $2009

Wavetable = $3000
Framebuffer = $4000

DMA_VX = $4000
DMA_VY = $4001
DMA_GX = $4002
DMA_GY = $4003
DMA_WIDTH = $4004
DMA_HEIGHT = $4005
DMA_Status = $4006
DMA_Color = $4007

;DMA flags are as follows
; 1   ->   DMA enabled
; 2   ->   Video out page
; 4   ->   NMI enabled
; 8   ->   G.RAM frame select
; 16  ->   V.RAM frame select
; 32  ->   CPU access bank select (High for V, low for G)
; 64  ->   Enable copy completion IRQ
; 128 ->   Transparency copy enabled (skips zeroes)

INPUT_MASK_UP		= %00001000
INPUT_MASK_DOWN		= %00000100
INPUT_MASK_LEFT		= %00000010
INPUT_MASK_RIGHT	= %00000001
INPUT_MASK_A		= %00010000
INPUT_MASK_B		= %00010000
INPUT_MASK_C		= %00100000
INPUT_MASK_START	= %00100000

	.org $E000
Inflate:
	.incbin "inflate_e000.obx"
RESET:
	LDX #$FF
	TXS
	SEI
	LDX #0
	LDY #0
StartupWait:
	DEX
	BNE StartupWait
	DEY
	BNE StartupWait

	LDA #2
	STA FrameFlip

	STZ ArrowIndex

	JSR InitAudioVars

	JSR CopyAudioRegisters

	;run INFLATE to decompress audio samples
	LDA #<Samples
	STA inflate_zp
	LDA #>Samples
	STA inflate_zp+1
	LDA #<Wavetable
	STA inflate_zp+2
	LDA #>Wavetable
	STA inflate_zp+3
	JSR Inflate

	;extract graphics to graphics RAM
	LDA #%00000000	;Activate lower page of VRAM/GRAM, CPU accesses GRAM, no IRQ, no transparency
	STA DMA_Flags
	
	;run INFLATE to decompress graphics
	LDA #<Sprites
	STA inflate_zp
	LDA #>Sprites
	STA inflate_zp+1
	LDA #<Framebuffer
	STA inflate_zp+2
	LDA #>Framebuffer
	STA inflate_zp+3
	JSR Inflate

Forever:
	JSR AwaitVSync
	LDA FrameFlip
	EOR #%00010010 ;flip bits 2 and 16
	STA FrameFlip
	ORA #%01100101
	STA DMA_Flags

	JSR UpdateInputs

	;;;;;;;;;;;;;;;;;;;;;;;;;;Diff button state against previous frame
	LDA Old_GamePad1BufferA
	EOR #$FF
	AND GamePad1BufferA
	STA tmp
	LDA Old_GamePad1BufferB
	EOR #$FF
	AND GamePad1BufferB
	STA tmp+1

	;;;;;;;;;;;;;;;;;;;;;;;;;;Test up and down buttons to move selection arrow
	LDA #INPUT_MASK_UP
	BIT tmp
	BEQ *+4
	DEC ArrowIndex

	LDA #INPUT_MASK_DOWN
	BIT tmp
	BEQ *+4
	INC ArrowIndex

	;;;;;;;;;;;;;;;;;;;;;;;;;;Keep arrow within 0-7 range
	LDA ArrowIndex
	AND #7
	STA ArrowIndex

	TAX

	;;;;;;;;;;;;;;;;;;;;;;;;;;Reset audio registers when start is pressed
	LDA #INPUT_MASK_START
	BIT tmp
	BEQ *+5
	JSR InitAudioVars


	;;;;;;;;;;;;;;;;;;;;;;;;;;If A is held, turbo remaining input tests
	LDA #INPUT_MASK_A
	BIT GamePad1BufferA
	BEQ *+6
	LDA GamePad1BufferB
	STA tmp+1
	;;;;;;;;;;;;;;;;;;;;;;;;;;Test left and right to adjust sound parameters
	LDA #INPUT_MASK_LEFT
	BIT tmp+1
	BEQ *+4
	DEC var_SquareNote1, x

	LDA #INPUT_MASK_RIGHT
	BIT tmp+1
	BEQ *+4
	INC var_SquareNote1, x

	JSR DrawBackground

	JSR DrawArrow

	;column 80
	;rows 16, 24, 40, 48, 64, 88, 96
	LDA #80
	STA PrintNum_X

	LDA #16
	STA PrintNum_Y
	LDA var_SquareNote1
	STA PrintNum_N
	JSR PrintNum
	LDA #24
	STA PrintNum_Y
	LDA var_SquareCtrl1
	STA PrintNum_N
	JSR PrintNum

	LDA #40
	STA PrintNum_Y
	LDA var_SquareNote2
	STA PrintNum_N
	JSR PrintNum
	LDA #48
	STA PrintNum_Y
	LDA var_SquareCtrl2
	STA PrintNum_N
	JSR PrintNum

	LDA #64
	STA PrintNum_Y
	LDA var_NoiseCtrl
	STA PrintNum_N
	JSR PrintNum

	LDA #88
	STA PrintNum_Y
	LDA var_WaveNote
	STA PrintNum_N
	JSR PrintNum
	LDA #96
	STA PrintNum_Y
	LDA var_WaveCtrl
	STA PrintNum_N
	JSR PrintNum

	JSR CopyAudioRegisters

	JMP Forever

AwaitVSync:
	LDA FrameFlag
	BNE	AwaitVSync
	LDA #1
	STA FrameFlag
	RTS

UpdateInputs:
	LDA GamePad1BufferA
	STA Old_GamePad1BufferA
	LDA GamePad1BufferB
	STA Old_GamePad1BufferB
	LDA GamePad2
	LDA GamePad1
	EOR #$FF
	STA GamePad1BufferA
	LDA GamePad1
	EOR #$FF
	STA GamePad1BufferB
	RTS

InitAudioVars:
	;init audio vars to do nothing
	LDA #0
	STA var_SquareNote1
	STA var_SquareNote2
	LDA #$94
	STA var_WaveNote
	LDA #63
	STA var_SquareCtrl1
	STA var_SquareCtrl2
	STA var_NoiseCtrl
	STA var_WaveCtrl
	RTS

CopyAudioRegisters:
	LDA var_SquareNote1
	STA SquareNote1
	LDA var_SquareCtrl1
	STA SquareCtrl1
	LDA var_SquareNote2
	STA SquareNote2
	LDA var_SquareCtrl2
	STA SquareCtrl2
	LDA var_NoiseCtrl
	STA NoiseCtrl
	LDA var_WaveNote
	STA WaveNote
	LDA var_WaveCtrl
	STA WaveCtrl
	RTS

DrawBackground:
	LDA #%01100101
	ORA FrameFlip
	STA DMA_Flags

	LDA #0
	STA DMA_VX
	LDA #0
	STA DMA_VY
	LDA #%10000000
	STA DMA_GX
	LDA #%00000000
	STA DMA_GY
	LDA #32
	STA DMA_WIDTH
	LDA #120
	STA DMA_HEIGHT
	
	LDA #$FF
	STA DMA_Color

	;start a DMA transfer
	LDA #1
	STA DMA_Status
	WAI

	LDA #32
	STA DMA_VX
	LDA #0
	STA DMA_VY
	LDA #32
	STA DMA_GX
	LDA #0
	STA DMA_GY
	LDA #95
	STA DMA_WIDTH
	LDA #120
	STA DMA_HEIGHT
	;start a DMA transfer
	LDA #1
	STA DMA_Status
	WAI
	RTS

DrawArrow:
	LDA #%11100101
	ORA FrameFlip
	STA DMA_Flags
	LDA #16
	STA DMA_VX
	LDY ArrowIndex
	LDA ArrowPositions, y
	STA DMA_VY
	LDA #0
	STA DMA_GX
	LDA #104
	STA DMA_GY
	LDA #15
	STA DMA_WIDTH
	LDA #16
	STA DMA_HEIGHT
	;start a DMA transfer
	LDA #1
	STA DMA_Status
	WAI
	RTS

PrintNum:
	LDA #%11100101
	ORA FrameFlip
	STA DMA_Flags
	;Print first digit
	LDA PrintNum_X
	STA DMA_VX
	LDA PrintNum_Y
	STA DMA_VY
	LDA PrintNum_N
	AND #$F0
	CLC
	LSR
	STA DMA_GX
	LDA #120
	STA DMA_GY
	LDA #7
	STA DMA_WIDTH
	LDA #8
	STA DMA_HEIGHT
	;start a DMA transfer
	LDA #1
	STA DMA_Status
	WAI
	;Print second digit
	LDA PrintNum_X
	CLC
	ADC #8
	STA DMA_VX
	LDA PrintNum_Y
	STA DMA_VY
	LDA PrintNum_N
	AND #$0F
	CLC
	ASL
	ASL
	ASL
	STA DMA_GX
	LDA #120
	STA DMA_GY
	LDA #7
	STA DMA_WIDTH
	LDA #8
	STA DMA_HEIGHT
	;start a DMA transfer
	LDA #1
	STA DMA_Status
	WAI
	RTS

Samples:
	.incbin "samples3.raw.deflate"
Sprites:
	.incbin "mainscreen.gtg.deflate"
ArrowPositions:
	.db 10, 18, 34, 42, 58, 66, 82, 90

NMI:
	STZ FrameFlag
	RTI

	.org $FFFA
	.dw NMI
	.dw RESET
	.dw 0