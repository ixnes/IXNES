#ifndef MMC1_H
#define MMC1_H

#include "Mapper.h"
#include <cstdint>

#define REG_CTRL			0x00
#define REG_CHR_BANK_0		0x01
#define REG_CHR_BANK_1		0x02
#define REG_PRG_BANK		0x03

#define MIR_ONESCREEN_LOW	0x00
#define MIR_ONESCREEN_HIGH	0x01
#define MIR_VERTICAL		0x02
#define MIR_HORIZONTAL		0x03

#define PRGMODE_16K			0x02
#define PRGMODE_FIX_LAST	0x01

#define	CHRMODE_8K			0x00
#define CHRMODE_4K			0x01
#define CHRMODE_RAM			0x02

//Forward declarations
class RomImage;
class Console;

class MMC1: public Mapper {
	Console *console;

	RomImage *rom;

	uint8_t *prgROM;

	uint16_t prgROMSize;

	uint8_t *chrROM;

	uint16_t chrROMSize;

	uint8_t *prgRAM;

	uint16_t prgRAMSize;

	uint8_t *chrRAM;

	uint16_t chrRAMSize;

	uint8_t *vram;

	uint16_t vramSize;

	uint8_t horizMirrorTable[4];

	uint8_t vertMirrorTable[4];

	uint8_t highMirrorTable[4];

	uint8_t lowMirrorTable[4];

	uint8_t *mirroringTable;

	uint8_t shiftRegister;

	int shiftCount;

	uint8_t chrBankMode;

	uint8_t chrBank0;

	uint8_t chrBank1;

	uint8_t prgBankMode;

	uint8_t prgBank;

	bool prgRAMDisabled;

	uint8_t writeLock;

	void writeRegister(uint16_t address, uint8_t data);

	uint32_t translatePrgRomAddress(uint16_t address);

	uint32_t translateChrRomAddress(uint16_t address);

public:
	MMC1(Console *con, RomImage *rom);

	~MMC1();

	uint8_t cpuRead(uint16_t address);

	uint8_t debugCpuRead(uint16_t address);

	void cpuWrite(uint16_t address, uint8_t data);

	uint8_t ppuRead(uint16_t address);

	uint8_t debugPpuRead(uint16_t address);

	void ppuWrite(uint16_t address, uint8_t data);

	void cycle();
};

#endif