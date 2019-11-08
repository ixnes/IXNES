#include "debug.h"
#include <cstdlib>
#include <sys/mman.h>
#include <iostream>
#include <fstream>

#define LOAD_ADDRESS 0x0000

int main(int argc, char *argv[]) {
	uint16_t startPC = 0x0400;

	cout << "Allocating memory..." << endl;
	uint8_t *memory = (uint8_t *) malloc(65536);

	/*DEBUG
	//Initializes start point to address 0x8000
	memory[0xFFFC] = startPC & 0xFF;
	memory[0xFFFD] = (startPC >> 4) & 0xFF;
	DEBUG*/

	//Loading file into memory
	cout << "Loading file: \"" << argv[1] << "\" into emulator..." << endl;
	ifstream file;
	file.open(argv[1], ios::in|ios::binary|ios::ate);
	//Checking if file opened successfully
	if (!file.is_open()) {
		cout << "Failed to open file. Exiting." << endl;
		return -1;
	}

	streampos size = file.tellg();
	file.seekg(0, ios::beg);
	file.read((char *)&memory[LOAD_ADDRESS], size);
	file.close();

	CPU cpu(memory);

	debug(cpu);
}