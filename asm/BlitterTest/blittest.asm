FrameFlag = $30

GamePad1BufferA = $3A
GamePad1BufferB = $3B
Old_GamePad1BufferA = $3C
Old_GamePad1BufferB = $3D
Dif_GamePad1BufferA = $3E
Dif_GamePad1BufferB = $3F

inflate_zp = $F0 ; F1, F2, F3
inflate_data = $0200 ; until $04FD

blitParams = $0500


Audio_Reset = $2000
Audio_NMI = $2001
Audio_Rate = $2006
DMA_Flags = $2007

GamePad1 = $2008
GamePad2 = $2009

INPUT_MASK_UP		= %00001000 ;Either
INPUT_MASK_DOWN		= %00000100 ;Either
INPUT_MASK_LEFT		= %00000010 ;B
INPUT_MASK_RIGHT	= %00000001 ;B
INPUT_MASK_A		= %00010000 ;A
INPUT_MASK_B		= %00010000 ;B
INPUT_MASK_C		= %00100000 ;B
INPUT_MASK_START	= %00100000 ;A

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

ARAM = $3000

Framebuffer = $4000
DMA = $4000
VX = $0
VY = $1
GX = $2
GY = $3
WIDTH = $4
HEIGHT = $5
STATUS = $6
COLOR = $7

    .org $E000
Inflate:
	.incbin "inflate_e000_0200.obx"
RESET:
    SEI
    STZ FrameFlag
    STZ GamePad1BufferA
	STZ GamePad1BufferB

    ;make sure audio coprocessor is stopped
	LDA #$7F
	STA Audio_Rate

    ;extract graphics to graphics RAM
	LDA #%10000000	;Activate lower page of VRAM/GRAM, CPU accesses GRAM, no IRQ, no transparency
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

    LDA #$10
    STA blitParams+VX
    STA blitParams+VY
    STA blitParams+GX
    STA blitParams+GY
    STA blitParams+WIDTH
    STA blitParams+HEIGHT
    
    LDA #%11100101
	STA DMA_Flags

    ;DMA flags are as follows
; 1   ->   DMA enabled
; 2   ->   Video out page
; 4   ->   NMI enabled
; 8   ->   G.RAM frame select
; 16  ->   V.RAM frame select
; 32  ->   CPU access bank select (High for V, low for G)
; 64  ->   Enable copy completion IRQ
; 128 ->   Transparency copy enabled (skips zeroes)

    ;main loop
Forever:
	JSR AwaitVSync
	JSR UpdateInputs
xx = $20
yy = $21
    STZ xx
    STZ yy
    LDA #INPUT_MASK_RIGHT
    BIT GamePad1BufferB
    BEQ *+4
    INC xx
    LDA #INPUT_MASK_LEFT
    BIT GamePad1BufferB
    BEQ *+4
    DEC xx
    LDA #INPUT_MASK_UP
    BIT GamePad1BufferA
    BEQ *+4
    DEC yy
    LDA #INPUT_MASK_DOWN
    BIT GamePad1BufferA
    BEQ *+4
    INC yy

    LDA #INPUT_MASK_A
    BIT GamePad1BufferA
    BEQ SkipA
    LDA blitParams+VX
    CLC
    ADC xx
    STA blitParams+VX
    LDA blitParams+VY
    CLC
    ADC yy
    STA blitParams+VY
SkipA:

    LDA #INPUT_MASK_B
    BIT GamePad1BufferB
    BEQ SkipB
    LDA blitParams+GX
    CLC
    ADC xx
    STA blitParams+GX
    LDA blitParams+GY
    CLC
    ADC yy
    STA blitParams+GY
SkipB:

    LDA #INPUT_MASK_C
    BIT GamePad1BufferB
    BEQ SkipC
    LDA blitParams+WIDTH
    CLC
    ADC xx
    STA blitParams+WIDTH
    LDA blitParams+HEIGHT
    CLC
    ADC yy
    STA blitParams+HEIGHT
SkipC:

    LDA blitParams+VX
    STA DMA+VX
    LDA blitParams+VY
    STA DMA+VY
    LDA blitParams+GX
    STA DMA+GX
    LDA blitParams+GY
    STA DMA+GY
    LDA blitParams+WIDTH
    STA DMA+WIDTH
    LDA blitParams+HEIGHT
    STA DMA+HEIGHT
    LDA #1
    STA DMA+STATUS
    STA DMA+STATUS
    WAI

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
	LDA Old_GamePad1BufferA
	EOR #$FF
	AND GamePad1BufferA
	STA Dif_GamePad1BufferA
	LDA Old_GamePad1BufferB
	EOR #$FF
	AND GamePad1BufferB
	STA Dif_GamePad1BufferB
	RTS

NMI:
	STZ FrameFlag
	RTI

IRQ:
    RTI

Sprites:
	.incbin "gamesprites.gtg.deflate"

	.org $FFFA
	.dw NMI
	.dw RESET
	.dw IRQ
