DAC = $8000

AccBuf = $00
Input = $02
ChIndex = $03
Freqs = $10
DecaysW = $20
DecaysF = $30
WaveStates = $40

	.org $0200
RESET:
	CLI
	LDA #$00
	STA $00
Forever:
	JMP Forever

IRQ:
	LDA #$FF

	DEC DecaysF+0 ;5
	BNE *+4 ;2/3
	DEC DecaysW+0 ;5/0
	BNE *+6 ;2/3
	STZ Freqs+0 ;3/0
	STA WaveStates+0


	DEC DecaysF+1 ;5
	BNE *+4 ;2/3
	DEC DecaysW+1 ;5/0
	BNE *+6 ;2/3
	STZ Freqs+1 ;3/0
	STA WaveStates+1

	DEC DecaysF+2 ;5
	BNE *+4 ;2/3
	DEC DecaysW+2 ;5/0
	BNE *+6 ;2/3
	STZ Freqs+2 ;3/0
	STA WaveStates+2

	DEC DecaysF+3 ;5
	BNE *+4 ;2/3
	DEC DecaysW+3 ;5/0
	BNE *+6 ;2/3
	STZ Freqs+3 ;3/0
	STA WaveStates+3

	STZ AccBuf ;3?

	CLC ;2
	LDA WaveStates+0 ;3
	ADC Freqs+0 ;3
	STA WaveStates+0 ;3
	ROL ;2
	LDA #$FF ;2
	ADC #$00 ;2
	AND DecaysW+0 ;3
	CLC ;2
	ADC AccBuf ;3
	STA AccBuf ;3

	CLC
	LDA WaveStates+1
	ADC Freqs+1
	STA WaveStates+1
	ROL
	LDA #$FF
	ADC #$00
	AND DecaysW+1
	CLC
	ADC AccBuf
	STA AccBuf

	CLC
	LDA WaveStates+2
	ADC Freqs+2
	STA WaveStates+2
	ROL
	LDA #$FF
	ADC #$00
	AND DecaysW+2
	CLC
	ADC AccBuf
	STA AccBuf

	CLC
	LDA WaveStates+3
	ADC Freqs+3
	STA WaveStates+3
	ROL
	LDA #$FF
	ADC #$00
	AND DecaysW+3
	CLC
	ADC AccBuf
	STA AccBuf

	LDA AccBuf ;3
	STA DAC ;3
	RTI ;6

NMI:
	INC ChIndex
	LDA ChIndex
	AND #3
	TAY
	LDA Input
	STA Freqs, y
	LDA #$40
	STA DecaysW, y
	LDA #$00
	STA DecaysF, y
	RTI


	.org $0FFA
	.dw NMI
	.dw RESET
	.dw IRQ