#include "Console.h"
#include "6502.h"
#include <cstdlib>
#include <sys/mman.h>
#include <iostream>
#include <fstream>
#include <string>

#define LOAD_ADDRESS 0x0000

static string getBinary(uint8_t n) {
	string ret = "";
	int mask = 0x80;
	for (int i = 0; i < 8; i++, mask >>= 1)
		if (n & mask)
			ret += "1";
		else
			ret += "0";
	return ret;
}

void debug(CPU& cpu) {
	string last ="";


	while (true) {
		string cmd;
		cout << "> ";
		cin >> cmd;

		if (cmd.compare("") == 0)
			cmd = last;

		//Execute next instruction
		if (cmd.compare("next") == 0 || cmd.compare("n") == 0) {
			//Print the info for the instruction that is about to be executed
			Operation op = cpu.performNextInstruction();
			cout << op.name << " ";
			//print processor status
			cout << hex << "\nX: " << +cpu.getX() << "\tY: " << +cpu.getY() << "\tACC: " << +cpu.getAcc() << endl;
			cout << hex << "PC:" << cpu.getProgramCounter() << "\tSP: " << +cpu.getStackPointer() << "\tP: " << getBinary(cpu.getStatus()) << endl;
		}
		else if (cmd.compare("cycle") == 0 || cmd.compare("c") == 0) {
			//Print the info for the instruction that is about to be executed
			cpu.cycle();
			//print processor status
			cout << hex << "X: " << +cpu.getX() << "\tY: " << +cpu.getY() << "\tACC: " << +cpu.getAcc() << endl;
			cout << hex << "PC:" << cpu.getProgramCounter() << "\tSP: " << +cpu.getStackPointer() << "\tP: " << getBinary(cpu.getStatus()) << endl;
		}
		else if (cmd.compare("status") == 0 || cmd.compare("s") == 0) {
			//print processor status
			cout << hex << "X: " << +cpu.getX() << "\tY: " << +cpu.getY() << "\tACC: " << +cpu.getAcc() << endl;
			cout << hex << "PC:" << cpu.getProgramCounter() << "\tSP: " << +cpu.getStackPointer() << "\tP: " << getBinary(cpu.getStatus()) << endl;
		}
		else if (cmd.compare("get") == 0) {
			uint16_t address;
			cout << "address > ";
			cin >> hex >> address;
			cout << hex << address << ": " << +cpu.readWord(address) << endl; 
		}
		else if (cmd.compare("run") == 0 || cmd.compare("r") == 0) {
			int cycles = 0;
			cout << "# instructions to run > ";
			cin >> cycles;
			for (int i = 0; i < cycles; i++)
				cpu.performNextInstruction();
			//print processor status
			cout << hex << "X: " << +cpu.getX() << "\tY: " << +cpu.getY() << "\tACC: " << +cpu.getAcc() << endl;
			cout << hex << "PC:" << cpu.getProgramCounter() << "\tSP: " << +cpu.getStackPointer() << "\tP: " << getBinary(cpu.getStatus()) << endl;

		}
		else if (cmd.compare("break") == 0 || cmd.compare("b") == 0) {
			uint16_t breakpoint = 0;
			cout << "Address to break on > ";
			cin >> hex >> breakpoint;
			while (cpu.getProgramCounter() != breakpoint)
				cpu.performNextInstruction();
			//print processor status
			cout << hex << "X: " << +cpu.getX() << "\tY: " << +cpu.getY() << "\tACC: " << +cpu.getAcc() << endl;
			cout << hex << "PC:" << cpu.getProgramCounter() << "\tSP: " << +cpu.getStackPointer() << "\tP: " << getBinary(cpu.getStatus()) << endl;

		}
		else if (cmd.compare("q") == 0) {
			break;
		}

		last = cmd;
	}
}

int main(int argc, char *argv[]) {
	uint16_t startPC = 0x0400;

	cout << "Allocating memory..." << endl;
	uint8_t *memory = (uint8_t *) malloc(65536);


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
	
	//Initializes start point to address 0x8000
	memory[0xFFFC] = 0x00;
	memory[0xFFFD] = 0x04;

	Console con(memory);

	debug(*con.getCPU());
}