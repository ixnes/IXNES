#ifndef NROM_H
#define NROM_H

#include "Mapper.h"
#include <cstdint>

//Forward declarations
class RomImage;
class Console;

class NROM: public Mapper {
	Console *console;

	RomImage *rom;

	uint8_t *prgROM;

	uint16_t prgROMSize;

	uint8_t *chrROM;

	uint16_t chrROMSize;

	uint8_t *prgRAM;

	uint16_t prgRAMSize;

	uint8_t *vram;

	uint16_t vramSize;

	uint8_t mirroringTable[4];

public:
	NROM(Console *con, RomImage *rom);

	~NROM();

	uint8_t cpuRead(uint16_t address);

	uint8_t debugCpuRead(uint16_t address);

	void cpuWrite(uint16_t address, uint8_t data);

	uint8_t ppuRead(uint16_t address);

	uint8_t debugPpuRead(uint16_t address);

	void ppuWrite(uint16_t address, uint8_t data);
};

#endif