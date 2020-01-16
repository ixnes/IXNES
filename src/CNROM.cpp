#include "CNROM.h"
#include "ROM.h"
#include "Console.h"

#include <iostream>

using namespace std;

uint8_t CNROM::cpuRead(uint16_t address) {
	if (address >= 0x8000) {
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

uint8_t CNROM::debugCpuRead(uint16_t address) {
	return cpuRead(address);
}

void CNROM::cpuWrite(uint16_t address, uint8_t data) {
	//PRG-ROM region
	if (address >= 0x8000) {
		chrROMBank = data;
	}

}

uint8_t CNROM::ppuRead(uint16_t address) {
	if (address < 0x2000) {
		cout << "Read address " << hex << address << " from bank " << +chrROMBank << endl;
		return chrROM[address + (0x2000 * (chrROMBank))];
	}
	else if (address < 0x3F00) {
		//calculate address after nametable mirroring
		uint8_t nametable = (address & 0x0C00) >> 10;
		address &= 0x03FF;
		address |= mirroringTable[nametable] << 10;
		return vram[address];
	}
}

uint8_t CNROM::debugPpuRead(uint16_t address) {
	return ppuRead(address);
}

void CNROM::ppuWrite(uint16_t address, uint8_t data) {
	if (address < 0x2000) {
		//Oops, ROM not RAM
		//chrROM[address | (0x2000 * chrROMBank)] = data;
	}
	else if (address < 0x3F00) {
		//calculate address after nametable mirroring
		uint8_t nametable = (address & 0x0C00) >> 10;
		address &= 0x03FF;
		address |= mirroringTable[nametable] << 10;
		vram[address] = data;
	}
}

CNROM::CNROM(Console *con, RomImage *rom) {
	this->console = con;

	this->rom = rom;

	prgROM = rom->getPrgRom();

	prgROMSize = rom->getPrgRomSize();

	chrROM = rom->getChrRom();

	chrROMSize = rom->getChrRomSize();

	chrROMBank = 0;

	vramSize = 0x800;

	vram = (uint8_t *) malloc(vramSize);

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

CNROM::~CNROM() {
	delete rom;
	delete vram;
}