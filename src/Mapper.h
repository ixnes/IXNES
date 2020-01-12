#ifndef MAPPER_H
#define MAPPER_H

#include <cstdint>

//Abstract base class for Mappers
class Mapper {
public:
	virtual uint8_t cpuRead(uint16_t address) = 0;

	virtual uint8_t debugCpuRead(uint16_t address) = 0;

	virtual void cpuWrite(uint16_t address, uint8_t data) = 0;

	virtual uint8_t ppuRead(uint16_t address) = 0;

	virtual uint8_t debugPpuRead(uint16_t address) = 0;

	virtual void ppuWrite(uint16_t address, uint8_t data) = 0;
};

#endif