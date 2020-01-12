#include "NROM.h"
#include "ROM.h"
#include "Console.h"

uint8_t NROM::cpuRead(uint16_t address) {
	//PRG-RAM region
	if (address < 0x8000 && address >= 0x6000) {
		if (prgRAMSize == 0)
			return console->getOpenBus();
		else 
			return prgRAM[(address - 0x6000) % prgRAMSize];
	}
	//PRG-ROM region
	else if (address >= 0x8000) {
		address -= 0x8000;
		//mirror if only 1 bank
		if (prgROMSize == 1) {
			address %= 0x4000;
		}
		return prgROM[address];
	}
	else {
		return console->getOpenBus();
	}
}

uint8_t NROM::debugCpuRead(uint16_t address) {
	return cpuRead(address);
}

void NROM::cpuWrite(uint16_t address, uint8_t data) {
	//PRG-RAM region
	if (address < 0x8000 && address >= 0x6000) {
		if (prgRAMSize != 0)
			prgRAM[(address - 0x6000) % prgRAMSize] = data;
	}
	//PRG-ROM region
	else if (address >= 0x8000) {
		address -= 0x8000;
		//mirror if only 1 bank
		if (prgROMSize == 1) {
			address %= 0x4000;
		}
		prgROM[address] = data;
	}

}

uint8_t NROM::ppuRead(uint16_t address) {
	if (address < 0x2000) {
		return chrROM[address];
	}
	else if (address < 0x3F00) {
		//calculate address after nametable mirroring
		uint8_t nametable = (address & 0x0C00) >> 10;
		address &= 0x03FF;
		address |= mirroringTable[nametable] << 10;
		return vram[address];
	}
}

uint8_t NROM::debugPpuRead(uint16_t address) {
	return ppuRead(address);
}

void NROM::ppuWrite(uint16_t address, uint8_t data) {
	if (address < 0x2000) {
		chrROM[address] = data;
	}
	else if (address < 0x3F00) {
		//calculate address after nametable mirroring
		uint8_t nametable = (address & 0x0C00) >> 10;
		address &= 0x03FF;
		address |= mirroringTable[nametable] << 10;
		vram[address] = data;
	}
}

NROM::NROM(Console *con, RomImage *rom) {
	this->console = con;

	this->rom = rom;

	prgROM = rom->getPrgRom();

	prgROMSize = rom->getPrgRomSize();

	chrROM = rom->getChrRom();

	chrROMSize = rom->getChrRomSize();

	vramSize = 0x800;

	vram = (uint8_t *) malloc(vramSize);

	prgRAMSize = rom->getPrgRamSize();

	if (prgRAMSize != 0) {
		prgRAM = (uint8_t *) malloc(prgRAMSize);
	}
	else {
		prgRAM = NULL;
	}

	if (rom->horizontalMirroring()) {
		mirroringTable[0] = 0;
		mirroringTable[1] = 0;
		mirroringTable[2] = 1;
		mirroringTable[3] = 1;
	}
	else {
		mirroringTable[0] = 0;
		mirroringTable[1] = 1;
		mirroringTable[2] = 0;
		mirroringTable[3] = 1;
	}
}

NROM::~NROM() {
	delete rom;
	delete vram;
	delete prgRAM;
}