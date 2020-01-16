#include "MMC1.h"
#include "ROM.h"
#include "Console.h"

void MMC1::writeRegister(uint16_t address, uint8_t data) {
	uint8_t reg = (address & 0x6000) >> 13;

	if (reg == REG_CTRL) {
		//Set mirror mode
		switch (data & 0x03) {
			case MIR_ONESCREEN_LOW:
				mirroringTable = lowMirrorTable;
				break;
			case MIR_ONESCREEN_HIGH:
				mirroringTable = highMirrorTable;
				break;
			case MIR_VERTICAL:
				mirroringTable = vertMirrorTable;
				break;
			case MIR_HORIZONTAL:
				mirroringTable = horizMirrorTable;
				break;
		}

		prgBankMode = (data & 0x0C) >> 2;

		chrBankMode = data & 0x10;
	}
	else if (reg == REG_CHR_BANK_0) {
		chrBank0 = data & 0x1F;
	}
	else if (reg == REG_CHR_BANK_1) {
		chrBank1 = data & 0x1F;
	}
	else if (reg == REG_PRG_BANK) {
		prgRAMDisabled = data & 0x10;
		prgBank = data & 0x0F;
	}
}

uint32_t MMC1::translatePrgRomAddress(uint16_t address) {
	//If in 16K bank mode
	if (prgBankMode & PRGMODE_16K) {
		//If looking at first slot
		if (address < 0xC000) {
			//If last slot is fixed, grab from switchable bank
			if (prgBankMode & PRGMODE_FIX_LAST)
				return (address - 0x8000) + (0x4000 * prgBank);
			//If first slot is fixed grab from first bank
			else
				return address - 0x8000;
		}
		//If looking at second slot
		else {
			//If last slot is fixed, grab from last bank
			if (prgBankMode & PRGMODE_FIX_LAST)
				return (address - 0xC000) + (0x4000 * (prgROMSize - 1));
			//If first slot is fixed grab from switchable bank
			else
				return (address - 0xC000) + (0x4000 * (prgBank & 0xFE));
		}

	}
	//If in 32K bank mode
	else {
		return (address - 0x8000) + (0x8000 * prgBank);
	}
}

uint32_t MMC1::translateChrRomAddress(uint16_t address) {
	//If in 4K bank mode
	if (chrBankMode == CHRMODE_4K) {
		//If looking at first slot
		if (address < 0x1000) {
			return address + (0x1000 * chrBank0);
		}
		//If looking at second slot
		else {
			return (address - 0x1000) + (0x1000 * chrBank1);
		}
	}
	//If in 8K bank mode
	else {
		return address + (0x2000 * (chrBank0 & 0xFE));
	}
}

uint8_t MMC1::cpuRead(uint16_t address) {
	//PRG-RAM region
	if (address < 0x8000 && address >= 0x6000) {
		if (prgRAMSize == 0 || prgRAMDisabled)
			return console->getOpenBus();
		else 
			return prgRAM[(address - 0x6000) % prgRAMSize];
	}
	//PRG-ROM region
	else if (address >= 0x8000) {
		return prgROM[translatePrgRomAddress(address)];
	}
	else {
		return console->getOpenBus();
	}
}

uint8_t MMC1::debugCpuRead(uint16_t address) {
	return cpuRead(address);
}

void MMC1::cpuWrite(uint16_t address, uint8_t data) {
	//PRG-RAM region
	if (address < 0x8000 && address >= 0x6000) {
		if (prgRAMSize != 0 && (!prgRAMDisabled))
			prgRAM[(address - 0x6000) % prgRAMSize] = data;
	}
	else if (address >= 0x8000) {
		if (data & 0x80) {
			shiftRegister = 0x00;
			shiftCount = 0;
		}
		else {
			if (!writeLock) {
				shiftRegister |= ((data & 0x01) << shiftCount);
				shiftCount++;
				if (shiftCount == 5) {
					writeRegister(address, data);
				}
				writeLock = 0x02;
			}
		}
	}
}

uint8_t MMC1::ppuRead(uint16_t address) {
	if (address < 0x2000) {
		if (chrBankMode == CHRMODE_RAM)
			return chrRAM[address % chrRAMSize];
		else
			return chrROM[translateChrRomAddress(address)];
	}
	else if (address < 0x3F00) {
		//calculate address after nametable mirroring
		uint8_t nametable = (address & 0x0C00) >> 10;
		address &= 0x03FF;
		address |= mirroringTable[nametable] << 10;
		return vram[address];
	}
}

uint8_t MMC1::debugPpuRead(uint16_t address) {
	return ppuRead(address);
}

void MMC1::ppuWrite(uint16_t address, uint8_t data) {
	if (address < 0x2000) {
		chrROM[translateChrRomAddress(address)] = data;
	}
	else if (address < 0x3F00) {
		//calculate address after nametable mirroring
		uint8_t nametable = (address & 0x0C00) >> 10;
		address &= 0x03FF;
		address |= mirroringTable[nametable] << 10;
		vram[address] = data;
	}
}

void MMC1::cycle() {
	writeLock >>= 1;
}

MMC1::MMC1(Console *con, RomImage *rom) {
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

	horizMirrorTable[0] = 0;
	horizMirrorTable[1] = 0;
	horizMirrorTable[2] = 1;
	horizMirrorTable[3] = 1;

	vertMirrorTable[0] = 0;
	vertMirrorTable[1] = 1;
	vertMirrorTable[2] = 0;
	vertMirrorTable[3] = 1;

	highMirrorTable[0] = 1;
	highMirrorTable[1] = 1;
	highMirrorTable[2] = 1;
	highMirrorTable[3] = 1;

	lowMirrorTable[0] = 0;
	lowMirrorTable[1] = 0;
	lowMirrorTable[2] = 0;
	lowMirrorTable[3] = 0;

	mirroringTable = horizMirrorTable;

	chrRAMSize = rom->getChrRamSize();

	if (chrRAMSize != 0) {
		chrRAM = (uint8_t *) malloc(chrRAMSize);
		chrBankMode = CHRMODE_RAM;
	}
	else {
		chrRAM = NULL;
		chrBankMode = 0x00;
	}

	shiftRegister = 0x00;

	shiftCount = 0;


	chrBank0 = 0x00;

	chrBank1 = 0x01;

	prgBankMode = 0x03;

	prgBank = 0x00;

	prgRAMDisabled = 0x00;
}

MMC1::~MMC1() {
	delete rom;
	delete vram;
	delete prgRAM;
}