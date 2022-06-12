Bank_Flags = $2005
DMA_Flags = $2007

;The two gamepad port addresses
GamePad1 = $2008
GamePad2 = $2009

Framebuffer = $4000
DMA_VX = $4000
DMA_VY = $4001
DMA_GX = $4002
DMA_GY = $4003
DMA_WIDTH = $4004
DMA_HEIGHT = $4005
DMA_Status = $4006
DMA_Color = $4007


;Define some macros to make register values more descriptive

;Input masks for the Gamepad registers
INPUT_MASK_UP		= %00001000 ;Always given
INPUT_MASK_DOWN		= %00000100 ;Always given
INPUT_MASK_LEFT		= %00000010 ;Only on second read
INPUT_MASK_RIGHT	= %00000001 ;Only on second read
INPUT_MASK_A		= %00010000 ;Only on first read
INPUT_MASK_B		= %00010000 ;Only on second read
INPUT_MASK_C		= %00100000 ;Only on second read
INPUT_MASK_START	= %00100000 ;Only on first read


;For DMA_Flags:
DMAMODE = 1
VID_OUT_PAGE2 = 2
VNMI_ENABLE = 4
COLORFILL = 8
NOTILE = 16
MAP_FRAMEBUFFER = 32
BLIT_IRQ = 64
OPAQUE = 128
;For Bank_Flags
VRAMBANK2 = 8
CLIP_X = 16
CLIP_Y = 32

;Color defines
HUE_GREEN = 0
HUE_YELLOW = 32
HUE_ORANGE = 64
HUE_RED = 96
HUE_MAGENTA = 128
HUE_INDIGO = 160
HUE_BLUE = 192
HUE_CYAN = 224

SAT_NONE = 0
SAT_SOME = 8
SAT_MORE = 16
SAT_FULL = 24

;tracking whether a vsync has occured
;will be set in NMI
did_vsync = $00

;mirrors for tracking Bank_Flags and DMA_Flags which are write-only
bank_mirror = $01
dma_mirror = $02

;Coordinates for our "character"
player_x = $03
player_y = $04
input_buffer = $05

    .org $E000

RESET:
    SEI

    ;Init registers
    LDA #1
	STA DMA_Flags
	STZ Bank_Flags


;Set up double buffering
    LDA #(DMAMODE | VNMI_ENABLE | NOTILE | BLIT_IRQ | COLORFILL);
    STA dma_mirror
    STA DMA_Flags

    LDA #VRAMBANK2
    STA bank_mirror
    STA Bank_Flags

Forever:


;Clear screen to black
    STZ DMA_VX
    STZ DMA_VY
    STZ DMA_GX
    STZ DMA_GY
    LDA #127
    STA DMA_WIDTH
    STA DMA_HEIGHT
    LDA #~8
    STA DMA_Color
    LDA #1
    STA DMA_Status

;Like in Example 03, we'll defer WAI until need to touch blitter registers.
;To fully read a gamepad port requires two successive reads of the same address.
;Reading from either gamepad address toggles its "select" pin.
;Also, reading from Gamepad1 will reset the select pin for Gamepad2 and visa-versa. So when reading
;from Gamepad1 we perform a read on Gamepad2 to put Gamepad1 into a known state.
    LDA GamePad2
    ;First read only exposes Up, Down, A, Start
    LDA GamePad1
    ;For the purpose of this demo we can use second read which exposes Up, Down, Left, Right, B, C
    LDA GamePad1
    ;controller buttons are grounded when pressed, so we invert for positive logic
    EOR #$FF
    STA input_buffer

;Check each cardinal direction and inc/decrement player_x, player_y
;Not particularly optimized approach is just BIT test each input mask
;and decide whether to skip an INC/DEC instruction
    LDA #INPUT_MASK_UP
    BIT input_buffer
    BEQ *+4 ;skips DEC player_y, which is 2 bytes because player_y is zeropage
    DEC player_y

    LDA #INPUT_MASK_DOWN
    BIT input_buffer
    BEQ *+4
    INC player_y

    LDA #INPUT_MASK_LEFT
    BIT input_buffer
    BEQ *+4
    DEC player_x

    LDA #INPUT_MASK_RIGHT
    BIT input_buffer
    BEQ *+4
    INC player_x

;The fill to black is almost certainly not done yet so we'll WAI now
;If you're unsure whether a blit will be done you may want to use an IRQ handler
;that sets a variable to track pending blits.
    WAI

    LDA player_x
    STA DMA_VX
    LDA player_y
    STA DMA_VY
    LDA #16
    STA DMA_WIDTH
    STA DMA_HEIGHT
    LDA #(HUE_RED | SAT_FULL | 4)
    EOR #$FF
    STA DMA_Color
    LDA #1
    STA DMA_Status
    WAI

;Last step is to swap the framebuffers, but let's wait for VSync first
AwaitVSync:
    LDA did_vsync
    BEQ AwaitVSync
    STZ did_vsync

;Using EOR to flip the bits for these flags
    LDA bank_mirror
    EOR #VRAMBANK2
    STA bank_mirror
    STA Bank_Flags

    LDA dma_mirror
    EOR #VID_OUT_PAGE2
    STA dma_mirror
    STA DMA_Flags

    JMP Forever

IRQ:
	RTI
NMI:
	INC did_vsync
	RTI

	.org $FFFA
	.dw NMI
	.dw RESET
	.dw IRQ