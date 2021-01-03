DAC = $8001
	.org $0200
RESET:
	CLI
	LDA #$80
	STA $00
Forever:
	JMP Forever

IRQ:
	NOP
	NOP
	CLC
	LDA $00
	ADC $02
	STA DAC
	STA $00
	RTI

NMI:
	RTI

	.org $0FFA
	.dw NMI
	.dw RESET
	.dw IRQ