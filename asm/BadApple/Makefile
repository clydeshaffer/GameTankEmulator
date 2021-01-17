badapple:
	vasm6502_oldstyle -dotdir -wdc02 -Fbin badapple.asm -o badapple.gtbs
	truncate -s 928235 tmp.padding
	cat bad_apple_frames.dat tmp.padding badapple.gtbs > badapple.gtr
	rm tmp.padding