#ifndef CONSOLE_H
#define CONSOLE_H

#include <cstdint>

#define CPU_RAM_SIZE 65535


//***********************************
//	Pretty much all of this code
//	is placeholder code for testing
//	the new CPU implementation
//***********************************


class CPU;

class Console {
private:

	CPU *cpu;

	uint8_t *ram;

public:
	Console(uint8_t *memory);

	~Console();

	uint8_t debugRead(uint16_t address);

	uint8_t cpuRead(uint16_t address);

	void cpuWrite(uint16_t address, uint8_t data);

	CPU *getCPU();
};

#endif