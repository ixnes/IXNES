#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <exception>
#include <string>

using namespace std;

class InvalidOpCodeException : public std::exception {
private:
	uint8_t opCode_;

public:
	InvalidOpCodeException(uint8_t opCode) {
		opCode_ = opCode_;
	}

	uint8_t getOpCode() {
		return opCode_;
	}
};

enum AddressMode { Implied, Implicit, Accumulator, Immediate, ZeroPage, ZeroPageX, 
									  ZeroPageY, Relative, Absolute, AbsoluteX,
									  AbsoluteY, Indirect, IndirectX, IndirectY };

class CPU;
class Console;

struct Operation {
	string name;
	uint8_t code;
	void (CPU::*opFunc)();
	AddressMode mode;
};

class CPU {
private:
	Console *console;

	//Registers (Refer to a 6502 manual for more information)
	uint16_t programCounter;
	uint8_t stackPointer;
	uint8_t status;
	uint8_t acc;
	uint8_t x;
	uint8_t y;

	//true if currently processing an instruction
	bool inInstruction;
	
	//true if currently setting up an interrupt
	bool inInterrupt;
	
	//true if the corresponding interrupt type is pending
	bool irqWaiting;
	bool nmiWaiting;
	bool resetWaiting;

	//Opcode of the instruction currently being processed
	uint8_t currentOp;

	//Address mode of instruction currently being processed
	AddressMode currentMode;

	//holds address and data information between cycles
	uint16_t addressTemp;
	uint16_t addressTempInd;
	int16_t  dataTemp;

	//used to track which 
	int cycleCounter;

	//lookup table for opcodes
	struct Operation ops[256];

	void setCarryFlag();

	void clearCarryFlag();

	bool getCarryFlag();

	void setZeroFlag();

	void clearZeroFlag();

	bool getZeroFlag();

	void setInterruptFlag();

	void clearInterruptFlag();

	bool getInterruptFlag();

	void setDecimalFlag();

	void clearDecimalFlag();

	bool getDecimalFlag();

	void setOverflowFlag();

	void clearOverflowFlag();

	bool getOverflowFlag();

	void setNegativeFlag();

	void clearNegativeFlag();

	bool getNegativeFlag();

	//called at the end of instruction execution to prepare
	//cpu to accept next instruction
	void exitInstruction();

	//This function is responsible for emulating the
	//sequence of loading the data for an operation
	//
	//Certain operations can skip a cycle under certain
	//addressing modes, if carrySkip is true, the function
	//which called is for one of those operations
	int16_t readData(bool carrySkip);

	//This function is responsible for emulating the
	//sequence of writing the data for an operation
	//
	//'data' is the data to be written in the effective address
	int16_t writeData(uint8_t data);

	//All branch instructions have the same cycle-by-cycle
	//behaviour except, obviously, for the branch condition
	//this is
	void branch();

	void opADC();

	void opAND();

	void opASL();

	void opBCC();

	void opBCS();

	void opBEQ();

	void opBIT();

	void opBMI();

	void opBNE();

	void opBPL();

	void opBRK();

	void opBVC();

	void opBVS();

	void opCLC();

	void opCLD();

	void opCLI();

	void opCLV();

	void opCMP();

	void opCPX();

	void opCPY();

	void opDEC();

	void opDEX();

	void opDEY();

	void opEOR();

	void opINC();

	void opINX();

	void opINY();

	void opJMP();

	void opJSR();

	void opLDA();

	void opLDX();

	void opLDY();

	void opLSR();

	void opNOP();

	void opORA();

	void opPHA();

	void opPHP();

	void opPLA();

	void opPLP();

	void opROL();

	void opROR();

	void opRTI();

	void opRTS();

	void opSBC();

	void opSEC();

	void opSED();

	void opSEI();

	void opSTA();

	void opSTX();

	void opSTY();

	void opTAX();

	void opTAY();

	void opTSX();

	void opTXA();

	void opTXS();


	void opTYA();

	//Handles an unrecognized opcode being read
	void opNotRecognized();

	//Sets one entry in the lookup tables
	void setLookup(int index, string name, void (CPU::*opFunc)(), AddressMode mode);

	//Initializes the lookup tables
	//Sets all unused codes to 0
	void initializeLookups();

	//Appropriate interrupt vector stored in addressTemp
	void performInterrupt();

	bool interruptWaiting();

public:
	CPU(Console *con);

	void cycle();

	void raiseIRQ();
	void raiseNMI();
	void raiseReset();

	//Debug functions
	uint8_t getX();
	void setX(uint8_t x);

	uint8_t getY();
	void setY(uint8_t y);

	uint8_t getAcc();
	void setAcc(uint8_t acc);

	uint8_t getStatus();
	void setStatus(uint8_t status);

	uint8_t getStackPointer();
	void setStackPointer(uint8_t stackPointer);

	uint16_t getProgramCounter();
	void setProgramCounter(uint16_t programCounter);

	Operation performNextInstruction();

	uint8_t readWord(uint16_t address);
};

#endif