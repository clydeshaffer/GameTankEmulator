	.org $E000
Inflate:
    .incbin "assets/inflate_e000_0200.obx"
;NOTICE! 05_audio.asm contains less than half the story on GameTank audio!
;To fully understand you must also check assets/square.asm which will run on the audio coprocessor (or "ACP")
AudioCode:
    .incbin "assets/square.o.deflate"

;pitches.dat is simply a lookup table we can use to convert MIDI note numbers
;into input values for our audio program
Pitches:
    .incbin "assets/pitches.dat"   

Bank_Flags = $2005
DMA_Flags = $2007

;These addresses map to SRAM shared with the ACP
;square_rate is a two-byte input to control a square wave generator
AudioRAM = $3000
square_rate = $3001
;square_rate+1 = $3002

;These registers control the ACP
;Writing to Audio_Reset will send a reset pulse to the ACP
;Audio_Rate pauses the ACP (bit 7) and sets the sample rate (bits 0-6)
;Higher values in Audio_Rate:0:6 give a lower sample rate.
;Specifically: A counter chip counts down from (Audio_Rate:0:6)*4 at 3.58MHz.
;Whenever this counter hits zero it triggers an audio sample and starts over.
Audio_Reset = $2000
Audio_Rate = $2006

;Again, labeling these zp addresses used by INFLATE
inflate_zp = $F0 ; F1, F2, F3

VNMI_ENABLE = 4

RESET:
    SEI
    LDA #VNMI_ENABLE
    STA DMA_Flags
    STZ Bank_Flags

    ;To start, we stop the ACP so that it doesn't glitch out while we're loading it
    LDA #$7F
	STA Audio_Rate

    ;Similarly to decompressing sprite data, we just point to AudioRAM and unpack AudioCode
    LDA #<AudioCode
	STA inflate_zp
	LDA #>AudioCode
	STA inflate_zp+1
	LDA #<AudioRAM
	STA inflate_zp+2
	LDA #>AudioRAM
	STA inflate_zp+3
	JSR Inflate

    ;Then activate the ACP by triggering a reset and deasserting its !Ready line
	STZ Audio_Reset
    LDA #$FF
	STA Audio_Rate

    ;Let's use $0 to count up a scale with a few frames in between.
    STZ $0

Forever:
    INC $0
    LDA $0
    CMP #108 ;Pitches.dat only has 108 entries
    BNE *+6
    LDA #0
    STA $0
    ASL ;Each entry in pitches.dat is 2 bytes
    TAX
    LDA Pitches+1, x
    STA square_rate
    LDA Pitches, x
    STA square_rate+1
    STZ Audio_Reset

    LDX #60
WaitLoop:
    DEX
    WAI
    BNE WaitLoop

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