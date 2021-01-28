temp = $10; $11

FrameFlag = $30

OctaveBuf      = $50
MusicPtr_Ch1   = $51 ; 52
MusicPtr_Ch2   = $53 ; 54
MusicPtr_Ch3   = $55 ; 56
MusicPtr_Ch4   = $57 ; 58
MusicNext_Ch1  = $59
MusicNext_Ch2  = $5A
MusicNext_Ch3  = $5B
MusicNext_Ch4  = $5C
MusicEnvI_Ch1   = $5D
MusicEnvI_Ch2   = $5E
MusicEnvI_Ch3   = $5F
MusicEnvI_Ch4   = $60
MusicEnvP_Ch1 = $61 ; 62
MusicEnvP_Ch2 = $63 ; 64
MusicEnvP_Ch3 = $65 ; 66
MusicEnvP_Ch4 = $67 ; 68
MusicStart_Ch1 = $69 ; 6A
MusicStart_Ch2 = $6B ; 6C
MusicStart_Ch3 = $6D ; 6E
MusicStart_Ch4 = $6F ; 70
MusicTicksTotal = $71 ; 72
MusicTicksLeft = $73 ; 74

inflate_zp = $F0 ; F1, F2, F3

inflate_data = $0200 ; until $04FD

LoadedMusicFirstPage = $4000

Audio_Reset = $2000
Audio_NMI = $2001
Audio_Rate = $2006

DMA_Flags = $2007

ARAM = $3000
LFSR = $04 ;$05
FreqsH = $10
FreqsL = $20
Amplitudes = $30

NoteDecay = 4

    .org $E000
Inflate:
	.incbin "inflate_e000_0200.obx"
RESET:

	;make sure audio coprocessor is stopped
	LDA #$7F
	STA Audio_Rate



    STZ MusicEnvI_Ch1
	STZ MusicEnvI_Ch2
	STZ MusicEnvI_Ch3
	STZ MusicEnvI_Ch4

	LDA #%01100100 ;load this hardcoded value into the A register
	STA DMA_Flags ;store the value from the A register into DMA_Flags

	LDA #<MusicPkg_Main
	STA inflate_zp
	LDA #>MusicPkg_Main
	STA inflate_zp+1
	JSR LoadMusic

    ;Unpack Audio Coprocessor Program
	LDA #<ACProg
	STA inflate_zp
	LDA #>ACProg
	STA inflate_zp+1
	LDA #<ARAM
	STA inflate_zp+2
	LDA #>ARAM
	STA inflate_zp+3
	JSR Inflate

	STZ Audio_Reset

	;enable RDY
	LDA #$FF
	STA Audio_Rate

Forever:
	JSR AwaitVSync

	;;;;Do music

	LDA MusicTicksLeft
	BNE DontLoopMusic
	LDA MusicTicksLeft+1
	BEQ RestartMusic
	DEC MusicTicksLeft+1
DontLoopMusic:
	DEC MusicTicksLeft
	JMP Music_SetCh1

RestartMusic:
	LDA MusicTicksTotal
	STA MusicTicksLeft
	LDA MusicTicksTotal+1
	STA MusicTicksLeft+1

	LDA MusicStart_Ch1
	STA MusicPtr_Ch1
	LDA MusicStart_Ch1+1
	STA MusicPtr_Ch1+1
	LDA #1
	STA MusicNext_Ch1

	LDA MusicStart_Ch2
	STA MusicPtr_Ch2
	LDA MusicStart_Ch2+1
	STA MusicPtr_Ch2+1
	LDA #1
	STA MusicNext_Ch2

	LDA MusicStart_Ch3
	STA MusicPtr_Ch3
	LDA MusicStart_Ch3+1
	STA MusicPtr_Ch3+1
	LDA #1
	STA MusicNext_Ch3

	LDA MusicStart_Ch4
	STA MusicPtr_Ch4
	LDA MusicStart_Ch4+1
	STA MusicPtr_Ch4+1
	LDA #1
	STA MusicNext_Ch4

Music_SetCh1:
	LDY MusicEnvI_Ch1
	LDA (MusicEnvP_Ch1), y
	AND #$80
	BNE *+4
	INC MusicEnvI_Ch1
	DEC MusicNext_Ch1
	BNE HoldNote_Ch1
	STZ MusicEnvI_Ch1
	INC MusicPtr_Ch1
	BNE *+4
	INC MusicPtr_Ch1+1
	LDY #0
	LDA (MusicPtr_Ch1), y
	STA MusicNext_Ch1
	INC MusicPtr_Ch1
	BNE *+4
	INC MusicPtr_Ch1+1
HoldNote_Ch1:
	LDY #0
	LDA (MusicPtr_Ch1), y
	BEQ Rest_Ch1
	JSR SetFreqAndOctave
	STA temp ; stash pitch low byte
	LDY MusicEnvI_Ch1
	LDA (MusicEnvP_Ch1), y
	PHA
	AND #$0F ;First four bits are pitch bend envelope
	CLC
	ADC #$F8 ;Midpoint is $08
	CLC
	ADC temp
	STA ARAM+FreqsL+0
	LDA OctaveBuf
	STA ARAM+FreqsH+0
	PLA
	AND #$70
	LSR
	STA ARAM+Amplitudes+0

	JMP Music_SetCh2
Rest_Ch1:
	STZ ARAM+FreqsL+0
	STZ ARAM+FreqsH+0
	STZ ARAM+Amplitudes+0

Music_SetCh2:
	LDY MusicEnvI_Ch2
	LDA (MusicEnvP_Ch2), y
	AND #$80
	BNE *+4
	INC MusicEnvI_Ch2
	DEC MusicNext_Ch2
	BNE HoldNote_Ch2
	STZ MusicEnvI_Ch2
	INC MusicPtr_Ch2
	BNE *+4
	INC MusicPtr_Ch2+1
	LDY #0
	LDA (MusicPtr_Ch2), y
	STA MusicNext_Ch2
	INC MusicPtr_Ch2
	BNE *+4
	INC MusicPtr_Ch2+1
HoldNote_Ch2:
	LDY #0
	LDA (MusicPtr_Ch2), y
	BEQ Rest_Ch2
	JSR SetFreqAndOctave
	STA temp ; stash pitch low byte
	LDY MusicEnvI_Ch2
	LDA (MusicEnvP_Ch2), y
	PHA
	AND #$0F ;First four bits are pitch bend envelope
	CLC
	ADC #$F8 ;Midpoint is $08
	CLC
	ADC temp
	STA ARAM+FreqsL+1
	LDA OctaveBuf
	STA ARAM+FreqsH+1
	PLA
	AND #$70
	LSR
	STA ARAM+Amplitudes+1

	JMP Music_SetCh3
Rest_Ch2:
	STZ ARAM+FreqsL+1
	STZ ARAM+FreqsH+1
	STZ ARAM+Amplitudes+1

Music_SetCh3:
	LDY MusicEnvI_Ch3
	LDA (MusicEnvP_Ch3), y
	AND #$80
	BNE *+4
	INC MusicEnvI_Ch3
	DEC MusicNext_Ch3
	BNE HoldNote_Ch3
	STZ MusicEnvI_Ch3
	INC MusicPtr_Ch3
	BNE *+4
	INC MusicPtr_Ch3+1
	LDY #0
	LDA (MusicPtr_Ch3), y
	STA MusicNext_Ch3
	INC MusicPtr_Ch3
	BNE *+4
	INC MusicPtr_Ch3+1
HoldNote_Ch3:
	LDY #0
	LDA (MusicPtr_Ch3), y
	BEQ Rest_Ch3
	JSR SetFreqAndOctave
	STA temp ; stash pitch low byte
	LDY MusicEnvI_Ch3
	LDA (MusicEnvP_Ch3), y
	PHA
	AND #$0F ;First four bits are pitch bend envelope
	CLC
	ADC #$F8 ;Midpoint is $08
	CLC
	ADC temp
	LDA #$FF
	STA ARAM+FreqsL+2
	LDA OctaveBuf
	STA ARAM+FreqsH+2
	PLA
	AND #$70
	ASL
	STA ARAM+Amplitudes+2

	JMP Music_SetCh4
Rest_Ch3:
	STZ ARAM+FreqsL+2
	STZ ARAM+FreqsH+2
	STZ ARAM+Amplitudes+2


Music_SetCh4:
	LDY MusicEnvI_Ch4
	LDA (MusicEnvP_Ch4), y
	AND #$80
	BNE *+4
	INC MusicEnvI_Ch4
	DEC MusicNext_Ch4
	BNE HoldNote_Ch4
	STZ MusicEnvI_Ch4
	INC MusicPtr_Ch4
	BNE *+4
	INC MusicPtr_Ch4+1
	LDY #0
	LDA (MusicPtr_Ch4), y
	STA MusicNext_Ch4
	INC MusicPtr_Ch4
	BNE *+4
	INC MusicPtr_Ch4+1
HoldNote_Ch4:
	LDY #0
	LDA (MusicPtr_Ch4), y
	BEQ Rest_Ch4
	JSR SetFreqAndOctave
	STA temp ; stash pitch low byte
	LDY MusicEnvI_Ch4
	LDA (MusicEnvP_Ch4), y
	PHA
	AND #$0F ;First four bits are pitch bend envelope
	CLC
	ADC #$F8 ;Midpoint is $08
	CLC
	ADC temp
	STA ARAM+FreqsL+3
	LDA OctaveBuf
	STA ARAM+FreqsH+3
	PLA
	AND #$70
	BEQ *+4
	LDA #$FF
	STA ARAM+Amplitudes+3

	JMP MusicDone
Rest_Ch4:
	STZ ARAM+FreqsL+3
	STZ ARAM+FreqsH+3
	STZ ARAM+Amplitudes+3

MusicDone:

    JMP Forever

LoadMusic:

	LDA #<LoadedMusicFirstPage
	STA inflate_zp+2
	LDA #>LoadedMusicFirstPage
	STA inflate_zp+3
	JSR Inflate

	LDA #<InstrumEnv1
	STA MusicEnvP_Ch1
	LDA #>InstrumEnv1
	STA MusicEnvP_Ch1+1
	LDA #$01
	STA MusicNext_Ch1

	LDA #<InstrumEnv2
	STA MusicEnvP_Ch2
	LDA #>InstrumEnv2
	STA MusicEnvP_Ch2+1
	LDA #$01
	STA MusicNext_Ch2

	LDA #<InstrumEnvSnare
	STA MusicEnvP_Ch3
	LDA #>InstrumEnvSnare
	STA MusicEnvP_Ch3+1
	LDA #$01
	STA MusicNext_Ch3

	LDA #<InstrumEnvSine
	STA MusicEnvP_Ch4
	LDA #>InstrumEnvSine
	STA MusicEnvP_Ch4+1
	LDA #$01
	STA MusicNext_Ch4

	;;Music pack header goes
	;LByte HByte - song length
	;LByte HByte - add to LoadedMusicFirstPage to get ch2 data
	;LByte HByte - add to LoadedMusicFirstPage to get ch3 data
	;LByte HByte - add to LoadedMusicFirstPage to get ch4 data
MusicHeaderLength = 8

	LDA #<(LoadedMusicFirstPage+MusicHeaderLength)
	STA MusicStart_Ch1
	LDA #>(LoadedMusicFirstPage+MusicHeaderLength)
	STA MusicStart_Ch1+1

	LDA #<LoadedMusicFirstPage
	CLC
	ADC LoadedMusicFirstPage+2 ;assuming here that the low byte of LoadedMusicFirstPage ptr is 00
	STA MusicStart_Ch2
	LDA #>LoadedMusicFirstPage
	CLC
	ADC LoadedMusicFirstPage+3
	STA MusicStart_Ch2+1

	LDA #<LoadedMusicFirstPage
	CLC
	ADC LoadedMusicFirstPage+4 ;assuming here that the low byte of LoadedMusicFirstPage ptr is 00
	STA MusicStart_Ch3
	LDA #>LoadedMusicFirstPage
	CLC
	ADC LoadedMusicFirstPage+5
	STA MusicStart_Ch3+1

	LDA #<LoadedMusicFirstPage
	CLC
	ADC LoadedMusicFirstPage+6 ;assuming here that the low byte of LoadedMusicFirstPage ptr is 00
	STA MusicStart_Ch4
	LDA #>LoadedMusicFirstPage
	CLC
	ADC LoadedMusicFirstPage+7
	STA MusicStart_Ch4+1

	LDA LoadedMusicFirstPage
	STA MusicTicksTotal
	LDA LoadedMusicFirstPage+1
	STA MusicTicksTotal+1

	STZ MusicTicksLeft
	STZ MusicTicksLeft+1
	RTS

SetFreqAndOctave:
	;This routine takes the command byte from the Accumulator and sets
	;the Accumulator to the pitch low byte and the OctaveBuf var to the corresponding pitch high byte
	;Uses the X and Y registers
	STA temp
	AND #$70
	LSR
	LSR
	LSR
	LSR
	TAX
	LDA TwelveTimesTable, x
	STA temp+1
	LDA temp
	AND #$0F
	CLC
	ADC temp+1
	ASL
	TAX
	LDA Pitches, x
	STA OctaveBuf
	LDA Pitches+1, x
	RTS

AwaitVSync:
	LDA FrameFlag
	BNE	AwaitVSync
	LDA #1
	STA FrameFlag
	RTS

ACProg:
	.incbin "dynawave.acp.deflate"

TwelveTimesTable:
	.db 0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120

Pitches:
	.incbin "pitches.dat"

InstrumEnv1:
	.db $58, $58, $58, $58, $48
	.db $48, $38, $38, $28, $08
	.db $88
InstrumEnv2:
	.db $5F, $5C, $48, $38, $38
	.db $38, $38, $38, $38, $38
	.db $28, $28, $28, $28, $28
	.db $18, $18, $18, $18, $18
	.db $88
InstrumEnvSnare:
	.db $28, $38, $48, $58, $68, $78
	.db $F8
InstrumEnvSine:
	.db $28, $26, $25, $24, $23
	.db $F8 

MusicPkg_Main:
	.incbin "bad-apple-v2_alltracks.gtm.deflate"

IRQ:
    RTI

NMI:    
	STZ FrameFlag
	RTI

	.org $FFFA
	.dw NMI
	.dw RESET
	.dw IRQ
