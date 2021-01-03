loader:
	vasm6502_oldstyle -dotdir -wdc02 -Fbin loader.asm -o acp_loader_test.gtr

player:
	vasm6502_oldstyle -dotdir -wdc02 -Fbin player.asm -o acp_player_test.gtr

acp_saw:
	vasm6502_oldstyle -dotdir -wdc02 -Fbin acp_saw_test.asm -o test.acp

acp_sine:
	vasm6502_oldstyle -dotdir -wdc02 -Fbin acp_sine_test.asm -o test.acp

acp_square:
	vasm6502_oldstyle -dotdir -wdc02 -Fbin acp_square_test.asm -o test.acp

acp_melodic:
	vasm6502_oldstyle -dotdir -wdc02 -Fbin acp_melodic.asm -o test.acp

acp_dynawave:
	vasm6502_oldstyle -dotdir -wdc02 -Fbin acp_dynawave.asm -o dynawave.acp

ROMFILE=undefined

flash:
	ls $(ROMFILE)
	cd ../../../eepromProgrammer ;\
	node index.js COM8 ../GameTankEmulator/asm/Audio2/$(ROMFILE)

flash_smd:
	ls $(ROMFILE)
	cd ../../../eepromProgrammer/scrambler ;\
	./scrambler.exe ../../GameTankEmulator/asm/Audio2/$(ROMFILE) ../../GameTankEmulator/asm/Audio2/$(ROMFILE).scrambled ;\
	cd .. ;\
	node index.js COM8 ../GameTankEmulator/asm/Audio2/$(ROMFILE).scrambled