Tmp = $00
GamePad1Last = $04
SrcPtr = $10
DestPtr = $12

CurrentNotes = $20; 21, 22, 23
CurrentAmps = $24; $25; $26; $27
NoteTimers = $28; 29, 2A, 2B
NoteLengths = $2C; 2D, 2E, 2F

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
LFSR = $04 ;$05
FreqsH = $10
FreqsL = $20
Amplitudes = $30

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
	LDA #$30
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

	STZ CurrentNotes+0
	STZ  CurrentAmps+0
	STZ CurrentNotes+1
	STZ  CurrentAmps+1
	STZ CurrentNotes+2
	STZ  CurrentAmps+2
	STZ CurrentNotes+3

	LDA #$FF
	STA CurrentAmps+3

Forever:
	LDY #0
BigDelay:
	JSR XDelay
	INY
	BNE BigDelay

	INC CurrentNotes+0
	DEC CurrentNotes+1
	INC CurrentNotes+3
	INC CurrentNotes+3

	DEC CurrentAmps+0
	DEC CurrentAmps+1

	LDA CurrentNotes+0
	CLC
	ASL
	TAX
	LDA Pitches, x
	STA ARAM+FreqsH+0
	LDA Pitches+1, x
	STA ARAM+FreqsL+0

	LDA CurrentNotes+1
	CLC
	ASL
	TAX
	LDA Pitches, x
	STA ARAM+FreqsH+1
	LDA Pitches+1, x
	STA ARAM+FreqsL+1

	LDA CurrentNotes+2
	CLC
	ASL
	TAX
	LDA Pitches, x
	STA ARAM+FreqsH+2
	LDA Pitches+1, x
	STA ARAM+FreqsL+2

	LDA CurrentNotes+3
	CLC
	ASL
	TAX
	LDA Pitches, x
	STA ARAM+FreqsH+3
	LDA Pitches+1, x
	STA ARAM+FreqsL+3

	LDA CurrentAmps+0
	LSR
	LSR
	STA ARAM+Amplitudes+0
	LDA CurrentAmps+1
	LSR
	STA ARAM+Amplitudes+1
	LDA CurrentAmps+2
	LSR
	LSR
	STA ARAM+Amplitudes+2
	LDA CurrentAmps+3
	STA ARAM+Amplitudes+3


	JMP Forever

XDelay:
	LDX #0
XLoop:
	INX
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
	.incbin "dynawave.acp"

Pitches:
	.icnbin "pitches.dat"

	.org $FFFA
	.dw NMI
	.dw RESET
	.dw IRQ

