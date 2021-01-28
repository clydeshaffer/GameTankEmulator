DAC = $8000

AccBuf = $00
LFSR = $04 ;$05
FreqsH = $10
FreqsL = $20
Amplitudes = $30
WaveStatesH = $50
WaveStatesL = $60

	.org $0000
	.db 0, 0, 0, 0, 2

	.org $0050
	.db $FF, $FF, $FF, $FF

	.org $0200
RESET:
	CLI
Forever:
	JMP Forever

IRQ:
	;Clear sum buffer
	STZ AccBuf ;3?

	;Channel 1 wavestate
	CLC ;2
	LDA WaveStatesL+0 ;3
	ADC FreqsL+0 ;3
	STA WaveStatesL+0
	LDA WaveStatesH+0
	ADC FreqsH+0
	STA WaveStatesH+0 ;3
	ROL ;2
	LDA #$FF ;2
	ADC #$00 ;2
	AND Amplitudes+0 ;3
	CLC ;2
	ADC AccBuf ;3
	STA AccBuf ;3

	;Channel 2 wavestate
	CLC ;2
	LDA WaveStatesL+1 ;3
	ADC FreqsL+1 ;3
	STA WaveStatesL+1
	LDA WaveStatesH+1
	ADC FreqsH+1
	STA WaveStatesH+1 ;3
	ROL ;2
	LDA #$FF ;2
	ADC #$00 ;2
	AND Amplitudes+1 ;3
	CLC ;2
	ADC AccBuf ;3
	STA AccBuf ;3

	;LFSR noise channel
	CLC ;2
	LDA WaveStatesL+2 ;3
	ADC FreqsL+2 ;3
	STA WaveStatesL+2
	LDA WaveStatesH+2
	ADC FreqsH+2
	STA WaveStatesH+2 ;3
	BCC AddNoise

	LDA LFSR
	ASL
	ROL LFSR+1
	BCC *+4
	EOR #$39
	STA LFSR
	LDA LFSR
	ASL
	ROL LFSR+1
	BCC *+4
	EOR #$39
	STA LFSR
	LDA LFSR
	ASL
	ROL LFSR+1
	BCC *+4
	EOR #$39
	STA LFSR
	LDA LFSR
	ASL
	ROL LFSR+1
	BCC *+4
	EOR #$39
	STA LFSR
	LDA LFSR
	ASL
	ROL LFSR+1
	BCC *+4
	EOR #$39
	STA LFSR
	LDA LFSR
	ASL
	ROL LFSR+1
	BCC *+4
	EOR #$39
	STA LFSR
	LDA LFSR
	ASL
	ROL LFSR+1
	BCC *+4
	EOR #$39
	STA LFSR
	LDA LFSR
	ASL
	ROL LFSR+1
	BCC *+4
	EOR #$39
	STA LFSR

AddNoise:
	LDA Amplitudes+2
	LSR
	LSR
	LSR
	LSR
	LSR
	BEQ SkipNoiseAttenuation
	TAX
	LDA LFSR
ShiftLoop:
	LSR
	DEX
	BNE ShiftLoop
SkipNoiseAttenuation:
	CLC ;2
	ADC AccBuf ;3
	STA AccBuf ;3
	
SineChannel:
	CLC ;2
	LDA WaveStatesL+3 ;3
	ADC FreqsL+3 ;3
	STA WaveStatesL+3
	LDA WaveStatesH+3
	ADC FreqsH+3
	STA WaveStatesH+3 ;3
	TAX
	LDA Sine, x
	LSR
	LSR
	AND Amplitudes+3 ; really only useful for muting here
	CLC
	ADC AccBuf

	;Move sum buffer to DAC
	;assuming final channel math ends with AccBuf in register A
	STA DAC ;3
	RTI ;6

NMI:
	RTI

	.align 8
Sine:
	.db 128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173
	.db 176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215
	.db 218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244
	.db 245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255
	.db 255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246
	.db 245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,220
	.db 218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179
	.db 176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131
	.db 128,124,121,118,115,112,109,106,103,100,97,93,90,88,85,82
	.db 79,76,73,70,67,65,62,59,57,54,52,49,47,44,42,40
	.db 37,35,33,31,29,27,25,23,21,20,18,17,15,14,12,11
	.db 10,9,7,6,5,5,4,3,2,2,1,1,1,0,0,0 
	.db 0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9
	.db 10,11,12,14,15,17,18,20,21,23,25,27,29,31,33,35
	.db 37,40,42,44,47,49,52,54,57,59,62,65,67,70,73,76
	.db 79,82,85,88,90,93,97,100,103,106,109,112,115,118,121,124

	.org $0FFA
	.dw NMI
	.dw RESET
	.dw IRQ