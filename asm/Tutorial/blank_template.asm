	.org $E000

RESET:
	;code here will get run once when the system boots


Forever:
	;code here will run over and over until the system is reset or turned off
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