Tmp = $00
GamePad1Last = $04
SrcPtr = $10
DestPtr = $12

inflate_zp = $F0 ; F1, F2, F3
inflate_data = $0200 ; until $04FD

Audio_Reset = $2000
Audio_NMI = $2001
Audio_Rate = $2006

GamePad1 = $2008
GamePad2 = $2009

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
ACPDest = $3200

	.org $E000
Inflate:
	.incbin "inflate_e000_0200.obx"
RESET:
	SEI
	;make sure audio coprocessor is stopped
	LDA #$7F
	STA Audio_Rate

	LDX #0
WaitForVIAReset:
	DEX
	BNE WaitForVIAReset
	LDA #$FF
	STA VIA+DDRB
	LDA #%01111000
	STA VIA+DDRA

	STZ VIA+ORB
	STZ VIA+ORA

	;unpack audio program to shared coprocessor RAM
	LDY #$0
	LDA #$32
	STA DestPtr+1
	STZ DestPtr
	LDA #<ACPProg
	STA SrcPtr
	LDA #>ACPProg
	STA SrcPtr+1
ARAMTestLoop:
	STY VIA+ORB
	LDA DestPtr+1
	ASL
	ASL
	ASL
	STA VIA+ORA
	LDA (SrcPtr), y
	STA (DestPtr), y
	LDA (DestPtr), y
	CMP (SrcPtr), y
	BNE *
	INY
	BNE ARAMTestLoop
	INC SrcPtr+1
	INC DestPtr+1
	LDA DestPtr+1
	CMP #$40
	BNE ARAMTestLoop

	STZ Audio_Reset

	;enable RDY
	LDA #$FF
	STA Audio_Rate

	NOP
	NOP
	LDA $3FFC
	STA VIA+ORB
	LDA $3FFD
	ASL
	ASL
	ASL
	STA VIA+ORA
	STZ Tmp
Forever:
	JSR XDelay
	LDA Tmp
	EOR #$FF
	STA GamePad1Last
	;code here will run over and over until the system is reset or turned off
	LDA GamePad2
	LDA GamePad1
	EOR #$FF
	AND #%00111100
	LSR
	LSR
	STA VIA
	STA Tmp
	AND GamePad1Last
	BEQ Forever
	LDA Tmp
	STA $3002
	STZ Audio_NMI
	JMP Forever

XDelay:
	LDX #0
XLoop:
	DEX
	BNE XLoop
	RTS


IRQ:
	;code here will run after certain interrupts (if enabled)
	;such as when a DMA draw call completes
	RTI
NMI:
	;code here will run once per frame (if enabled)
	RTI

	.align 8
ACPProg:
	.incbin "test.acp"

	.org $FFFA
	.dw NMI
	.dw RESET
	.dw IRQ