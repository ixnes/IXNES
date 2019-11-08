#include "6502.h"
#include <string.h>
#include <iostream>

using namespace std;

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
			Operation op = cpu.getCurrentOp();
			cout << op.name << " ";
			if (op.bytes == 2)
				cout << hex << +cpu.readWord(cpu.getProgramCounter() + 1);
			else if (op.bytes == 3 || op.code == 0x4C || op.code == 0x6C || op.code == 0x20)
				cout << hex << cpu.readDoubleWord(cpu.getProgramCounter() + 1);
			cout << endl;
			//Execute instruction
			cpu.performNextInstruction();
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
			cout << "# cycles to run > ";
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

			//Print the info for the instruction that is about to be executed
			cout << "Next instruction:" << endl;
			Operation op = cpu.getCurrentOp();
			cout << op.name << " ";
			if (op.bytes == 2)
				cout << hex << +cpu.readWord(cpu.getProgramCounter() + 1);
			else if (op.bytes == 3 || op.code == 0x4C || op.code == 0x6C || op.code == 0x20)
				cout << hex << cpu.readDoubleWord(cpu.getProgramCounter() + 1);
			cout << endl;
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