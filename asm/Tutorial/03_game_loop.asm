Bank_Flags = $2005
DMA_Flags = $2007

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


inflate_zp = $F0 ; F1, F2, F3

;tracking whether a vsync has occured
;will be set in NMI
did_vsync = $00

;mirrors for tracking Bank_Flags and DMA_Flags which are write-only
bank_mirror = $01
dma_mirror = $02

;counter to use in game loop for animation
frame_count = $03


    .org $E000
Inflate:
	.incbin "assets/inflate_e000_0200.obx"
SpriteSheet:
	.incbin "assets/cubicle.gtg.deflate"

RESET:
    ;Not doing anything with the IRQ handler
    ;WAI can still activate on blit completion
    SEI

    ;Init registers
    LDA #1
	STA DMA_Flags
	STZ Bank_Flags

    ;Put DMA params in known state
	STZ DMA_GX
	STZ DMA_GY
	STA DMA_WIDTH
	STA DMA_HEIGHT
	STA DMA_Status
	STZ DMA_Status
	STZ DMA_Flags

    STZ frame_count
    STZ did_vsync
    ;Unpack sprite data
    LDA #<SpriteSheet
	STA inflate_zp
	LDA #>SpriteSheet
	STA inflate_zp+1
	LDA #<Framebuffer
	STA inflate_zp+2
	LDA #>Framebuffer
	STA inflate_zp+3
	JSR Inflate

;Our logic and render loop will end with flipping the VID_OUT_PAGE bit
; on DMA_Flags and the VRAMBANK bit on Bank_flags. So at any given time
; we will be drawing to the opposite framebuffer from the one currently
; on screen.
; Initial values enable DMA, enable NMI generation on VSync,
; disable tile mode, and enable IRQ on blit completion.
; Note the absense of VID_OUT_PAGE2, so first framebuffer is showing on screen.
    LDA #(DMAMODE | VNMI_ENABLE | NOTILE | BLIT_IRQ);
    STA dma_mirror
    STA DMA_Flags
; Initial bank flag is VRAMBANK2 and both CLIP flags
; VRAMBANK2 means that DMA writes to second framebuffer
; CLIP_X and CLIP_Y prevent a draw near the edge from warping
;   to the other side of the screen.
    LDA #(VRAMBANK2 | CLIP_X | CLIP_Y)
    STA bank_mirror
    STA Bank_Flags

Forever:


;Let's start by clearing the screen to black

;First set colorfill
    LDA dma_mirror
    ORA #COLORFILL
    STA dma_mirror
    STA DMA_Flags

;Then setup a full screen draw with black
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

;We could WAI now to wait for it to finish, but it
;isn't actually needed until we need to touch any
;DMA/Banking registers. So let's run some logic in the meantime!

;Run cycle from "Cubicle Knight" is three 16x16 frames and
;starts at (16,32). We'll use a counter to cycle over this range

    LDA frame_count
    INC
    CMP #32
    BNE *+4
    LDA #8
    STA frame_count
    ASL
    AND #$F0

;Sicne we didn't before, we need to WAI before we touch DMA registers.
    WAI
    STA DMA_GX

;Set the rest of the DMA values.
    LDA #32
    STA DMA_GY

;Draw the character at (12, 34)
    LDA #12
    STA DMA_VX
    LDA #34
    STA DMA_VY

;Character sprite is 16x16
    LDA #16
    STA DMA_WIDTH
    STA DMA_HEIGHT

;Unset colorfill
    LDA dma_mirror
    AND #~COLORFILL
    STA dma_mirror
    STA DMA_Flags

;Start the drawing
    LDA #1
    STA DMA_Status

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