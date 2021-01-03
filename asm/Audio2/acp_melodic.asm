DAC = $8000

AccBuf = $00
Input = $02
ChIndex = $03
FreqsW = $10
FreqsF = $20
DecaysW = $30
DecaysF = $40
WaveStatesW = $50
WaveStatesF = $60

	.org $0200
RESET:
	CLI
	LDA #$00
	STA $00
Forever:
	JMP Forever

IRQ:
	;Clear sum buffer
	STZ AccBuf ;3?

	;Advance channel decays
	LDA #$FF
	;Channel 1 decay
	DEC DecaysF+0 ;5
	BNE *+4 ;2/3
	DEC DecaysW+0 ;5/0
	BNE *+10 ;2/3
	STZ FreqsW+0 ;3/0
	STZ FreqsF+0
	STA WaveStatesW+0
	STZ WaveStatesF+0

	;Channel 2 decay
	DEC DecaysF+1 ;5
	BNE *+4 ;2/3
	DEC DecaysW+1 ;5/0
	BNE *+10 ;2/3
	STZ FreqsW+1 ;3/0
	STZ FreqsF+1
	STA WaveStatesW+1
	STZ WaveStatesF+1

	;Channel 1 wavestate
	CLC ;2
	LDA WaveStatesF+0 ;3
	ADC FreqsF+0 ;3
	STA WaveStatesF+0
	LDA WaveStatesW+0
	ADC FreqsW+0
	STA WaveStatesW+0 ;3
	ROL ;2
	LDA #$FF ;2
	ADC #$00 ;2
	AND DecaysW+0 ;3
	CLC ;2
	ADC AccBuf ;3
	STA AccBuf ;3

	;Channel 2 wavestate
	CLC ;2
	LDA WaveStatesF+1 ;3
	ADC FreqsF+1 ;3
	STA WaveStatesF+1
	LDA WaveStatesW+1
	ADC FreqsW+1
	STA WaveStatesW+1 ;3
	ROL ;2
	LDA #$FF ;2
	ADC #$00 ;2
	AND DecaysW+1 ;3
	CLC ;2
	ADC AccBuf ;3
	STA AccBuf ;3

	;Move sum buffer to DAC
	LDA AccBuf ;3
	STA DAC ;3
	RTI ;6

NMI:
	INC ChIndex
	LDA ChIndex
	AND #1 ;only cycle through two channels in case new method too slow
	TAY
	LDX Input
	LDA NotesF, x
	STA FreqsF, y
	LDA NotesW, x
	STA FreqsW, y
	LDA #$40
	STA DecaysW, y
	LDA #$00
	STA DecaysF, y
	RTI

NotesF:
	.db $CA, $13, $60, $B2, $09, $65, $C6, $2D, $9A, $0E, $89, $0B, $94, $26, $C1, $64
NotesW:
	.db $04, $05, $05, $05, $06, $06, $06, $07, $07, $08, $08, $09, $09, $0A, $0A, $0B

	.org $0FFA
	.dw NMI
	.dw RESET
	.dw IRQ