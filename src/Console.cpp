#include "Console.h"
#include "6502.h"

#include <iostream>

Console::Console(uint8_t *memory) {
	cpu = new CPU(this);

	cpu->raiseReset();

	ram = memory;
}

uint8_t Console::debugRead(uint16_t address) {
	return ram[address];
}

uint8_t Console::cpuRead(uint16_t address) {
	return ram[address];
}

void Console::cpuWrite(uint16_t address, uint8_t data) {
	//cout << "Write:" << endl;
	//cout << "Address\t" << hex << address << endl;
	//cout << "Data\t" << hex << +data << endl; 
	ram[address] = data;
}

CPU *Console::getCPU() {
	return cpu;
}

Console::~Console() {
	delete cpu;
	delete ram;
}