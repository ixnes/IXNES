#include <cstdint>
#include <exception>
#include <string>
#include <iostream>

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

struct Operation {
	string name;
	uint8_t code;
	uint8_t bytes;
	uint8_t cycles;
};

class CPU {
private:
	struct Operation ops[256];

	/*
	//Lookups for byte length and number of cycles for every opcode
	uint8_t opByteLength[256];
	uint8_t opCycleLength[256];
	string opName[256];
	*/

	//Registers (Refer to a 6502 manual for more information)
	uint16_t programCounter_;
	uint8_t stackPointer_;
	uint8_t status_;
	uint8_t acc_;
	uint8_t x_;
	uint8_t y_;

	//Pointer to field of memory
	//Should be 64k
	uint8_t *memory_;

	//Tracks how many clock cycles have passed since the beginning  of execution
	uint32_t cycles;

	//void writeWord(uint16_t address, uint8_t data);

	//uint8_t readWord(uint16_t address);

	//uint16_t readDoubleWord(uint16_t address);

	void setCarryFlag() {
		status_ |= 0x01;
	}

	void clearCarryFlag() {
		status_ &= 0xFE;
	}

	bool getCarryFlag() {
		return status_ & 0x01;
	}

	void setZeroFlag() {
		status_ |= 0x02;
	}

	void clearZeroFlag() {
		status_ &= 0xFD;
	}

	bool getZeroFlag() {
		return status_ & 0x02;
	}

	void setInterruptFlag() {
		status_ |= 0x04;
	}

	void clearInterruptFlag() {
		status_ &= 0xFB;
	}

	bool getInterruptFlag() {
		return status_ & 0x04;
	}

	void setDecimalFlag() {
		status_ |= 0x08;
	}

	void clearDecimalFlag() {
		status_ &= 0xF7;
	}

	bool getDecimalFlag() {
		return status_ & 0x08;
	}

	//Okay, so TIL the 'break flag' isn't actually a hardware flag,
	//it's something that only exists when the status word is pushed
	//to stack by an interrupt sequence
	/*void setBreakFlag() {
		status_ |= 0x10;
	}

	void clearBreakFlag() {
		status_ &= 0xEF;
	}

	bool getBreakFlag() {
		return status_ & 0x10;
	}*/

	void setOverflowFlag() {
		status_ |= 0x40;
	}

	void clearOverflowFlag() {
		status_ &= 0xBF;
	}

	bool getOverflowFlag() {
		return status_ & 0x40;
	}

	void setNegativeFlag() {
		status_ |= 0x80;
	}

	void clearNegativeFlag() {
		status_ &= 0x7F;
	}

	bool getNegativeFlag() {
		return status_ & 0x80;
	}


	void setArithmeticFlags(uint8_t op1, uint8_t op2, uint16_t result) {
		//Sets carry flag if overflow in bit 7
		if (result > 255)
			setCarryFlag();
		else
			clearCarryFlag();

		//Sets zero flag is result is zero
		if (result == 0)
			setZeroFlag();
		else
			clearZeroFlag();

		//Sets overflow flag if sign bit is incorrect
		//This big, complicated logic statement first checks if the operands have the
		//same sign (if they have different signs it can't overflow), and then checks
		//if the result have the same sign as the operands. If it does not, then
		//overflow occurred and the overflow flag is set.
		if (! ((op1 & op2 & 0x80) && (op1 & result & 0x80)) )
			setOverflowFlag();
		else
			clearOverflowFlag();

		//Sets negative flag is result is negative
		if (result & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();
	}

	//This function is just a basic read, but by making the argument 8-bit
	//it provides convienient built-in wraparound for indexed zero page reads
	uint8_t readZeroPage(uint8_t address) {
		return memory_[address];
	}

	uint16_t readZeroPageDouble(uint8_t address) {
		//6502 is little endian, so we make sure that the bytes are ordered
		//properly before returning
		return memory_[address] | (memory_[address+1] << 8);
	}

	//equivalent of readZeroPage, but for writing
	void writeZeroPage(uint8_t address, uint8_t data) {
		memory_[address] = data;
	}

	void pushToStack(uint8_t data) {
		writeWord(0x100+stackPointer_, data);
		stackPointer_--;
	}

	void pushToStackDouble(uint16_t data) {
		writeWord(0x100+stackPointer_, (data >> 8) & 0xFF);
		stackPointer_--;
		writeWord(0x100+stackPointer_, data & 0xFF);
		stackPointer_--;
	}

	uint8_t popFromStack() {
		stackPointer_++;
		return readWord(0x100+stackPointer_);
	}

	uint16_t popFromStackDouble() {
		stackPointer_ +=  2;
		return readDoubleWord(0x100+stackPointer_-1);
	}

	//This function handles page cross checks for indexed absolute reads
	//It is necessary because if the address calculation crosses a page boundary
	//then the instruction takes an extra cycle
	//This is because the address is calculated by adding the index to the
	//lower byte of the address (probably because the processor just uses
	//its normal 8-bit adder circuits) and if there is a carry then the higher
	//order byte (the page byte) must also be altered
	void pageCrossCheck(uint16_t baseAddress, uint8_t index) {
		if ( ((baseAddress + index) & 0xFF00) != (baseAddress & 0xFF00) )
			cycles++;
	}

	//Sets one entry in the lookup tables
	void setLookup(int index, string name, uint8_t bytes, uint8_t cycles) {
		ops[index].name = name;
		ops[index].bytes = bytes;
		ops[index].cycles = cycles;
	}

	//Initializes the lookup tables
	//Sets all unused codes to 0
	void initializeLookups() {
		for (uint16_t i = 0; i < 256; i++) {
			ops[i].code = i;
			ops[i].bytes = 0;
			ops[i].cycles = 0;
		}

		//ADC Immediate
		setLookup(0x69, "ADC Immediate", 2, 2);

		//ADC zero page
		setLookup(0x65, "ADC Zero Page", 2, 3);

		//ADC zero page x
		setLookup(0x75, "ADC Zero Page,X", 2, 4);

		//ADC absolute
		setLookup(0x6D, "ADC Absolute", 3, 4);

		//ADC absolute x
		setLookup(0x7D, "ADC Absolute,X", 3, 4);

		//ADC absolute y
		setLookup(0x79, "ADC Absolute,Y", 3, 4);

		//ADC indirect x
		setLookup(0x61, "ADC (Indirect,X)", 2, 6);

		//ADC indirect y
		setLookup(0x71, "ADC (Indirect),Y", 2, 5);

		//AND immediate
		setLookup(0x29, "AND Immediate", 2, 2);

		//AND zero page
		setLookup(0x25, "AND Zero Page", 2, 3);

		//AND zero page x
		setLookup(0x35, "AND Zero Page,X", 2, 4);

		//AND absolute
		setLookup(0x2D, "AND Absolute", 3, 4);

		//AND absolute x
		setLookup(0x3D, "AND Absolute,X", 3, 4);

		//AND absolute y
		setLookup(0x39, "AND Absolute,Y", 3, 4);

		//AND indirect x
		setLookup(0x21, "AND (Indirect,X)", 2, 6);

		//AND indirect y
		setLookup(0x31, "AND (Indirect),Y", 2, 5);

		//ASL accumulator
		setLookup(0x0A, "ASL Accumulator", 1, 2);

		//ASL zero page
		setLookup(0x06, "ASL Zero Page", 2, 5);

		//ASL zero page x
		setLookup(0x16, "ASL Zero Page,X", 2, 6);

		//ASL absolute
		setLookup(0x0E, "ASL Absolute", 3, 6);

		//ASL absolute x
		setLookup(0x1E, "ASL Absolute,X", 3, 7);

		//BCC relative
		setLookup(0x90, "BCC Relative", 2, 2);

		//BCS relative
		setLookup(0xB0, "BCS Relative", 2, 2);

		//BEQ relative
		setLookup(0xF0, "BEQ Relative", 2, 2);

		//BIT zero page
		setLookup(0x24, "BIT Zero Page", 2, 3);

		//BIT absolute
		setLookup(0x2C, "BIT Absolute", 3, 4);

		//BMI relative
		setLookup(0x30, "BMI Relative", 2, 2);

		//BNE relative
		setLookup(0xD0, "BNE Relative", 2, 2);

		//BPL relative
		setLookup(0x10, "BPL Relative", 2, 2);

		//BRK implied
		//Bytes set to 0 avoid screwing up program counter
		setLookup(0x00, "BRK Implied", 0, 7);

		//BVC relative
		setLookup(0x50, "BVC Relative", 2, 2);

		//BVS relative
		setLookup(0x70, "BVS Relative", 2, 2);

		//CLC implied
		setLookup(0x18, "CLC Implied", 1, 2);

		//CLD implied
		setLookup(0xD8, "CLD Implied", 1, 2);//Bytes set to 0 avoid screwing up program counter

		//CLI implied
		setLookup(0x58, "CLI Implied", 1, 2);

		//CLV implied
		setLookup(0xB8, "CLV Implied", 1, 2);

		//CMP immediate
		setLookup(0xC9, "CMP Immediate", 2, 2);

		//CMP zero page
		setLookup(0xC5, "CMP Zero Page", 2, 3);

		//CMP zero page x
		setLookup(0xD5, "CMP Zero Page,X", 2, 4);

		//CMP absolute
		setLookup(0xCD, "CMP Absolute", 3, 4);

		//CMP absolute x
		setLookup(0xDD, "CMP Absolute,X", 3, 4);

		//CMP absolute y
		setLookup(0xD9, "CMP Absolute,Y", 3, 4);

		//CMP indirect x
		setLookup(0xC1, "CMP (Indirect,X)", 2, 6);

		//CMP indirect y
		setLookup(0xD1, "CMP (Indirect),Y", 2, 5);

		//CPX immediate
		setLookup(0xE0, "CPX Immediate", 2, 2);

		//CPX zero page
		setLookup(0xE4, "CPX Zero Page", 2, 3);

		//CPX absolute
		setLookup(0xEC, "CPX Absolute", 3, 4);

		//CPY immediate
		setLookup(0xC0, "CPY Immediate", 2, 2);

		//CPY zero page
		setLookup(0xC4, "CPY Zero Page", 2, 3);

		//CPY absolute
		setLookup(0xCC, "CPY Absolute", 3, 4);

		//DEC zero page
		setLookup(0xC6, "DEC Zero Page", 2, 5);

		//DEC zero page x
		setLookup(0xD6, "DEC Zero Page,X", 2, 6);

		//DEC absolute
		setLookup(0xCE, "DEC Absolute", 3 , 6);

		//DEC absolute x
		setLookup(0xDE, "DEC Absolute,X", 3, 7);

		//DEX implied
		setLookup(0xCA, "DEX Implied", 1, 2);

		//DEY implied
		setLookup(0x88, "DEY Implied", 1, 2);

		//EOR immediate
		setLookup(0x49, "EOR Immediate", 2, 2);

		//EOR zero page
		setLookup(0x45, "EOR Zero Page", 2, 3);

		//EOR zero page x
		setLookup(0x55, "EOR Zero Page,X", 2, 4);

		//EOR absolute
		setLookup(0x4D, "EOR Absolute", 3, 4);

		//EOR absolute x
		setLookup(0x5D, "EOR Absolute,X", 3, 4);

		//EOR absolute y
		setLookup(0x59, "EOR Absolute,Y", 3, 4);

		//EOR Indirect x
		setLookup(0x41, "EOR (Indirect,X)", 2, 6);

		//EOR indirect y
		setLookup(0x51, "EOR (Indirect),Y", 2, 5);

		//INC zero page
		setLookup(0xE6, "INC Zero Page", 2, 5);

		//INC zero page x
		setLookup(0xF6, "INC Zero Page,X", 2, 6);

		//INC absolute
		setLookup(0xEE, "INC Absolute", 3, 6);

		//INC absolute x
		setLookup(0xFE, "INC Absolute,X", 3, 7);

		//INX implied
		setLookup(0xE8, "INX Implied", 1, 2);

		//INY implied
		setLookup(0xC8, "INY Implied", 1, 2);

		//JMP absolute
		//Bytes set to 0 avoid screwing up program counter
		setLookup(0x4C, "JMP Absolute", 0, 3);

		//JMP indirect
		//Bytes set to 0 avoid screwing up program counter
		setLookup(0x6C, "JMP Indirect", 0, 5);

		//JSR absolute
		//Bytes set to 0 avoid screwing up program counter
		setLookup(0x20, "JSR Absolute", 0, 6);

		//LDA immediate
		setLookup(0xA9, "LDA Immediate", 2, 2);

		//LDA zero page
		setLookup(0xA5, "LDA Zero Page", 2, 3);

		//LDA zero page x
		setLookup(0xB5, "LDA Zero Page,X", 2, 4);

		//LDA absolute
		setLookup(0xAD, "LDA Absolute", 3, 4);

		//LDA absolute x
		setLookup(0xBD, "LDA Absolute,X", 3, 4);

		//LDA absolute y
		setLookup(0xB9, "LDA Absolute,Y", 3, 4);

		//LDA indirect x
		setLookup(0xA1, "LDA (Indirect,X)", 2, 6);

		//LDA indirect y
		setLookup(0xB1, "LDA (Indirect),Y", 2, 5);

		//LDX immediate
		setLookup(0xA2, "LDX Immediate", 2, 2);

		//LDX zero page
		setLookup(0xA6, "LDX Zero Page", 2, 3);

		//LDX zero page y
		setLookup(0xB6, "LDX Zero Page,Y", 2, 4);

		//LDX absolute
		setLookup(0xAE, "LDX Absolute", 3, 4);

		//LDX absolute y
		setLookup(0xBE, "LDX Absolute,Y", 3, 4);

		//LDY immediate
		setLookup(0xA0, "LDY Immediate", 2, 2);

		//LDY zero page
		setLookup(0xA4, "LDY Zero Page", 2, 3);

		//LDY zero page x
		setLookup(0xB4, "LDY Zero Page,X", 2, 4);

		//LDY absolute
		setLookup(0xAC, "LDY Absolute", 3, 4);

		//LDY absolute x
		setLookup(0xBC, "LDY Absolute,X", 3, 4);

		//LSR accumulator
		setLookup(0x4A, "LSR Accumulator", 1, 2);

		//LSR zero page
		setLookup(0x46, "LSR Zero Page", 2, 5);

		//LSR zero page x
		setLookup(0x56, "LST Zero Page,X", 2, 6);

		//LSR absolute
		setLookup(0x4E, "LSR Absolute", 3, 6);

		//LSR absolute x
		setLookup(0x5E, "LSR Absolute,X", 3, 7);

		//NOP implied
		setLookup(0xEA, "NOP Implied", 1, 2);

		//ORA immediate
		setLookup(0x09, "ORA Immediate", 2, 2);

		//ORA zero page
		setLookup(0x05, "ORA Zero Page", 2, 3);

		//ORA zero page x
		setLookup(0x15, "ORA Zero Page,X", 2, 4);

		//ORA absolute
		setLookup(0x0D, "ORA Absolute", 3, 4);

		//ORA absolute x
		setLookup(0x1D, "ORA Absolute,X", 3, 4);

		//ORA absolute y
		setLookup(0x19, "ORA Absolute,Y", 3, 4);

		//ORA indirect x
		setLookup(0x01, "ORA (Indirect,X)", 2, 6);

		//ORA indirect y
		setLookup(0x11, "ORA (Indirect),Y", 2, 5);

		//PHA implied
		setLookup(0x48, "PHA Implied", 1, 3);

		//PHP implied
		setLookup(0x08, "PHP Implied", 1, 3);

		//PLA implied
		setLookup(0x68, "PLA Implied", 1 , 4);

		//PLP implied
		setLookup(0x28, "PLP Implied", 1, 4);

		//ROL accumulator
		setLookup(0x2A, "ROL Accumulator", 1, 2);

		//ROL zero page
		setLookup(0x26, "ROL Zero Page", 2, 5);

		//ROL zero page x
		setLookup(0x36, "ROL Zero Page,X", 2, 6);

		//ROL absolute
		setLookup(0x2E, "ROL Absolute", 3, 6);

		//ROL absolute x
		setLookup(0x3E, "ROL Absolute,X", 3, 7);

		//ROR accumulator
		setLookup(0x6A, "ROR Accumulator", 1, 2);

		//ROR zero page
		setLookup(0x66, "ROR Zero Page", 2, 5);

		//ROR zero page x
		setLookup(0x76, "ROR Zero Page,X", 2, 6);

		//ROR absolute
		setLookup(0x6E, "ROR Absolute", 3, 6);

		//ROR absolute x
		setLookup(0x7E, "ROR Absolute,X", 3, 7);

		//RTI implied
		//Bytes set to 0 avoid screwing up program counter
		setLookup(0x40, "RTI Implied", 0, 6);

		//RTS implied
		setLookup(0x60, "RTS Implied", 1, 6);

		//SBC immediate
		setLookup(0xE9, "SBC Immediate", 2, 2);

		//SBC zero paeg
		setLookup(0xE5, "SBC Zero Page", 2, 3);

		//SBC zero page x
		setLookup(0xF5, "SBC Zero Page,X", 2, 4);

		//SBC absolute
		setLookup(0xED, "SBC Absolute", 3, 4);

		//SBC absolute x
		setLookup(0xFD, "SBC Absolute,X", 3, 4);

		//SBC absolute y
		setLookup(0xF9, "SBC Absolute,Y", 3, 4);

		//SBC indirect x
		setLookup(0xE1, "SBC (Indirect,X)", 2, 6);

		//SBC indirect y
		setLookup(0xF1, "SBC (Indirect),Y", 2, 5);

		//SEC implied
		setLookup(0x38, "SEC Implied", 1, 2);

		//SED implied
		setLookup(0xF8, "SED Implied", 1, 2);

		//SEI implied
		setLookup(0x78, "SEI Implied", 1, 2);

		//STA zero page
		setLookup(0x85, "STA Zero page", 2, 3);

		//STA zero page x
		setLookup(0x95, "STA Zero Page,X", 2, 4);

		//STA absolute
		setLookup(0x8D, "STA Absolute", 3, 4);

		//STA absolute x
		setLookup(0x9D, "STA Absolute,X", 3, 5);

		//STA absolute y
		setLookup(0x99, "STA Absolute,Y", 3, 5);

		//STA indirect x
		setLookup(0x81, "STA (Indirect,X)", 2, 6);

		//STA indirect y
		setLookup(0x91, "STA (Indirect),Y", 2, 6);

		//STX zero page
		setLookup(0x86, "STX Zero Page", 2, 3);

		//STX zero page y
		setLookup(0x96, "STX Zero Page,Y", 2, 4);

		//STX absolute
		setLookup(0x8E, "STX Absolute", 3, 4);

		//STY zero page
		setLookup(0x84, "STX Zero Page", 2, 3);

		//STY zero page x
		setLookup(0x94, "STY Zero Page,X", 2, 4);

		//STY absolute
		setLookup(0x8C, "STY Absolute", 3, 4);

		//TAX implied
		setLookup(0xAA, "TAX Implied", 1, 2);

		//TAY implied
		setLookup(0xA8, "TAY Implied", 1, 2);

		//TSX implied
		setLookup(0xBA, "TSX Implied", 1, 2);

		//TXA implied
		setLookup(0x8A, "TXA Implied", 1, 2);

		//TXS implied
		setLookup(0x9A, "TXS Implied", 1, 2);

		//TYA implied
		setLookup(0x98, "TYA Implied", 1 , 2);
	}

	void opADC(uint8_t data) {
		int16_t resultUnsigned = acc_ + data + (getCarryFlag() ? 1 : 0);
		int16_t resultSigned   = ((int8_t)acc_) + ((int8_t)data) + (getCarryFlag() ? 1 : 0);

		//setArithmeticFlags(acc_, operand, result);

		acc_ = resultUnsigned;

		//Sets carry flag if overflow in bit 7
		if (resultUnsigned > 255)
			setCarryFlag();
		else
			clearCarryFlag();

		//Sets zero flag is result is zero
		if (acc_ == 0)
			setZeroFlag();
		else
			clearZeroFlag();

		//Sets overflow flag if sign bit is incorrect
		//This big, complicated logic statement first checks if the operands have the
		//same sign (if they have different signs it can't overflow), and then checks
		//if the result have the same sign as the operands. If it does not, then
		//overflow occurred and the overflow flag is set.
		//if (! ((acc_ & operand & 0x80) && (acc_ & result & 0x80)) )
			

		//Sets overflow flag if result cannot be represented in
		//an 8-bit signed integer
		if (resultSigned > 127 || resultSigned < -128)
			setOverflowFlag();
		else
			clearOverflowFlag();

		//Sets negative flag is result is negative
		if (resultUnsigned & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();
	}

	void opAND(uint8_t operand) {
		acc_ &= operand;

		//Sets zero flag if result is zero
		if (acc_ == 0)
			setZeroFlag();
		else
			clearZeroFlag();

		//Sets negative flag is result is negative
		if (acc_ & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();
	}

	void opASL(uint16_t address) {
		uint8_t data = readWord(address);

		//Grab the high bit for the carry flag
		if (data & 0x80)
			setCarryFlag();
		else
			clearCarryFlag();
		data <<= 1;

		//Sets zero flag if result is zero
		if (data == 0)
			setZeroFlag();
		else
			clearZeroFlag();

		//Sets negative flag is result is negative
		if (data & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		writeWord(address, data);
	}

	void opBIT(uint8_t data) {
		//set V and N to bits 6 and 7 respectively
		//don't claim to understand what the purpose is here
		//but the reference says this is what happens
		if (data & 0x40)
			setOverflowFlag();
		else
			clearOverflowFlag();

		if (data & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		if ((acc_ & data) == 0)
			setZeroFlag();
		else
			clearZeroFlag();
	}

	void opCMP(uint8_t data) {
		if (acc_ >= data)
			setCarryFlag();
		else
			clearCarryFlag();

		if (acc_ == data)
			setZeroFlag();
		else
			clearZeroFlag();

		if ((acc_-data) & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();
	}

	void opCPX(uint8_t data) {
		if (x_ >= data)
			setCarryFlag();
		else
			clearCarryFlag();

		if (x_ == data)
			setZeroFlag();
		else
			clearZeroFlag();

		if ((x_-data) & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();
	}

	void opCPY(uint8_t data) {
		if (y_ >= data)
			setCarryFlag();
		else
			clearCarryFlag();

		if (y_ == data)
			setZeroFlag();
		else
			clearZeroFlag();

		if ((y_-data) & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();
	}

	void opDEC(uint16_t address) {
		uint8_t data = readWord(address) - 1;

		if (data == 0)
			setZeroFlag();
		else
			clearZeroFlag();

		if (data & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		writeWord(address, data);
	}

	void opEOR(uint8_t operand) {
		acc_ ^= operand;

		//Sets zero flag if result is zero
		if (acc_ == 0)
			setZeroFlag();
		else
			clearZeroFlag();

		//Sets negative flag is result is negative
		if (acc_ & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();
	}

	void opINC(uint16_t address) {
		uint8_t data = readWord(address) + 1;

		if (data == 0)
			setZeroFlag();
		else
			clearZeroFlag();

		if (data & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		writeWord(address, data);
	}

	void opLDA(uint8_t data) {
		acc_ = data;

		//Sets zero flag if result is zero
		if (acc_ == 0)
			setZeroFlag();
		else
			clearZeroFlag();

		//Sets negative flag is result is negative
		if (acc_ & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();
	}

	void opLDX(uint8_t data) {
		x_ = data;

		//Sets zero flag if result is zero
		if (x_ == 0)
			setZeroFlag();
		else
			clearZeroFlag();

		//Sets negative flag is result is negative
		if (x_ & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();
	}

	void opLDY(uint8_t data) {
		y_ = data;

		//Sets zero flag if result is zero
		if (y_ == 0)
			setZeroFlag();
		else
			clearZeroFlag();

		//Sets negative flag is result is negative
		if (y_ & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();
	}

	void opLSR(uint16_t address) {
		uint8_t data = readWord(address);

		//Grab the low bit for the carry flag
		if (data & 0x01)
			setCarryFlag();
		else
			clearCarryFlag();

		data = (data >> 1) & 0x7F;

		//Sets zero flag if result is zero
		if (data == 0)
			setZeroFlag();
		else
			clearZeroFlag();

		//Sets negative flag is result is negative
		if (data & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		writeWord(address, data);
	}

	void opORA(uint8_t operand) {
		acc_ |= operand;

		//Sets zero flag if result is zero
		if (acc_ == 0)
			setZeroFlag();
		else
			clearZeroFlag();

		//Sets negative flag is result is negative
		if (acc_ & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();
	}

	void opROL(uint16_t address) {
		uint8_t data = readWord(address);

		uint8_t tmp = data & 0x80;
		data <<= 1;

		if (getCarryFlag())
			data |= 0x01;
		else
			data &= 0xFE;

		if (tmp)
			setCarryFlag();
		else
			clearCarryFlag();

		if (data == 0)
			setZeroFlag();
		else
			clearZeroFlag();

		if (data & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		writeWord(address, data);
	}

	void opROR(uint16_t address) {
		uint8_t data = readWord(address);

		uint8_t tmp = data & 0x01;
		data >>= 1;

		if (getCarryFlag())
			data |= 0x80;
		else
			data &= 0x7F;

		if (tmp)
			setCarryFlag();
		else
			clearCarryFlag();

		if (data == 0)
			setZeroFlag();
		else
			clearZeroFlag();

		if (data & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		writeWord(address, data);
	}

	void opSBC(uint8_t data) {
		//This ungodly ternary operator is because the 6502 does subtraction
		//in a weird way to support multibyte substraction
		//If you just want to subtract one byte you have to 
		//make sure the carry flag is set before you perform it
		int16_t resultUnsigned = acc_  - data - (getCarryFlag() ? 0 : 1);
		int16_t resultSigned   = ((int8_t)acc_) - ((int8_t)data) - (getCarryFlag() ? 0 : 1);

		//setArithmeticFlags(acc_, (-1 * data) - (getCarryFlag() ? 0 : 1), result);

		acc_ = resultUnsigned;

		//Sets carry flag is the result is greater than or equal to zero
		if (resultUnsigned >= 0)
			setCarryFlag();
		else
			clearCarryFlag();

		//Sets zero flag is result is zero
		if (acc_ == 0)
			setZeroFlag();
		else
			clearZeroFlag();

		//Sets overflow flag if sign bit is incorrect
		//This big, complicated logic statement first checks if the operands have the
		//same sign (if they have different signs it can't overflow), and then checks
		//if the result have the same sign as the operands. If it does not, then
		//overflow occurred and the overflow flag is set.
		/*if (! ((acc_ & data & 0x80) && (acc_ & result & 0x80)) )
			setOverflowFlag();
		else
			clearOverflowFlag();*/

		//Sets overflow flag if result cannot be represented as 8-bit signed integer
		if (resultSigned > 127 || resultSigned < -128)
			setOverflowFlag();
		else
			clearOverflowFlag();

		//Sets negative flag is result is negative
		if (resultUnsigned & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();
	}

public:
	//Writes data byte to memory location pointed to by address
	void writeWord(uint16_t address, uint8_t data) {
		memory_[address] = data;
	}

	//Returns the byte at the address
	uint8_t readWord(uint16_t address) {
		return memory_[address];
	}

	//Returns the two bytes at the address
	uint16_t readDoubleWord(uint16_t address) {
		//6502 is little endian, so we make sure that the bytes are ordered
		//properly before returning
		return memory_[address] | (memory_[address+1] << 8);
	}

	//Creates a CPU object and initializes it to be ready to start execution
	CPU(uint8_t *memory) {
		//Initializes the lookup tables
		initializeLookups();

		//Sets memory pointer to given address
		memory_ = memory;

		//These two bytes in the memory, FFFC and FFFD, contain the value the
		//program counter is supposed to be initialized to
		//FFFC contains the lower half and FFFD contains the upper half
		//programCounter_ = memory_[0xFFFC] | (memory_[0xFFFD] << 8);

		//DEBUG
		programCounter_ = 0x0400;

		//Sets stack pointer to top of stack area
		//Not totally sure if this is necessary, in fact I'm pretty sure it's not
		//but it can't hurt
		stackPointer_ = 0xFF;

		//Interrupt flag is  set by default when the processor starts up
		setInterruptFlag();
	}

	//Executes the next instruction pointed to by the program counter
	void performNextInstruction() {
		uint8_t opCode = readWord(programCounter_);

		switch (opCode) {
		//ADC (Add memory to accumulator with carry)
			//Immediate
			case 0x69:
				opADC(readWord(programCounter_+1));
				break;
			//Zero page
			case 0x65:
				opADC(readZeroPage(readWord(programCounter_+1)));
				break;
			//Zero page,X
			case 0x75:
				opADC(readZeroPage(readWord(programCounter_+1)+x_));
				break;
			//Absolute
			case 0x6D:
				opADC(readWord(readDoubleWord(programCounter_+1)));
				break;
			//Absolute,X
			case 0x7D:
				pageCrossCheck(readDoubleWord(programCounter_+1), x_);

				opADC(readWord(readDoubleWord(programCounter_+1) + x_));
				break;
			//Absolute, Y
			case 0x79:
				pageCrossCheck(readDoubleWord(programCounter_+1), y_);

				opADC(readWord(readDoubleWord(programCounter_+1) + y_));
				break;
			//(Indirect,X)
			case 0x61:
				opADC(readWord(readZeroPageDouble(readWord(programCounter_+1) + x_)));
				break;
			//(Indirect),Y
			case 0x71:
				pageCrossCheck(readZeroPageDouble(readWord(programCounter_+1)), y_);

				opADC(readWord(readZeroPageDouble(readWord(programCounter_+1)) + y_));
				break;
		//AND (Logical AND)
			//Immediate
			case 0x29:
				opAND(readWord(programCounter_+1));
				break;
			//Zero page
			case 0x25:
				opAND(readZeroPage(readWord(programCounter_+1)));
				break;
			//Zero page x
			case 0x35:
				opAND(readZeroPage(readWord(programCounter_+1) + x_));
				break;
			//Absolute
			case 0x2D:
				opAND(readWord(readDoubleWord(programCounter_+1)));
				break;
			//Absolute X
			case 0x3D:
				pageCrossCheck(readDoubleWord(programCounter_+1), x_);

				opAND(readWord(readDoubleWord(programCounter_+1) + x_));
				break;
			//Absolute Y
			case 0x39:
				pageCrossCheck(readDoubleWord(programCounter_+1), y_);

				opAND(readWord(readDoubleWord(programCounter_+1) + y_));
				break;
			//Indirect x
			case 0x21:
				opAND(readWord(readZeroPageDouble(readWord(programCounter_+1) + x_)));
				break;
			//Indirect y
			case 0x31:
				pageCrossCheck(readZeroPageDouble(readWord(programCounter_+1)), y_);

				opAND(readWord(readZeroPageDouble(readWord(programCounter_+1)) + y_));
				break;
		//ASL (Arithmetic shift left)
			//Accumulator
			case 0x0A:
				//Grab the high bit for the carry flag
				if (acc_ & 0x80)
					setCarryFlag();
				else
					clearCarryFlag();

				acc_ <<= 1;

				//Sets zero flag if result is zero
				if (acc_ == 0)
					setZeroFlag();
				else
					clearZeroFlag();

				//Sets negative flag is result is negative
				if (acc_ & 0x80)
					setNegativeFlag();
				else
					clearNegativeFlag();
				break;
			//Zero Page
			case 0x06:
				opASL(readWord(programCounter_+1));
				break;
			//Zero Page,X
			case 0x16:
				opASL((readWord(programCounter_+1) + x_) & 0xFF);
				break;
			//Absolute
			case 0x0E:
				opASL(readDoubleWord(programCounter_+1));
				break;
			//Absolute,X
			case 0x1E:
				opASL(readDoubleWord(programCounter_+1) + x_);
				break;
		//BCC (branch if carry clear)
			//Relative
			case 0x90:
				if (!getCarryFlag()) {
					pageCrossCheck(programCounter_+2, readWord(programCounter_+1));
					cycles++;
					programCounter_ += (int8_t) readWord(programCounter_+1);
				}

				break;
		//BCS (branch if carry set)
			//Relative
			case 0xB0:
				if (getCarryFlag()) {
					pageCrossCheck(programCounter_+2, readWord(programCounter_+1));
					cycles++;
					programCounter_ += (int8_t) readWord(programCounter_+1);
				}

				break;
		//BEQ (branch if equal)
			//Relative
			case 0xF0:
				if (getZeroFlag()) {
					pageCrossCheck(programCounter_+2, readWord(programCounter_+1));
					cycles++;
					programCounter_ += (int8_t) readWord(programCounter_+1);
				}

				break;
		//BIT (bit test)
			//Zero page
			case 0x24:
				opBIT(readZeroPage(readWord(programCounter_ + 1)));
				break;
			//Absolute
			case 0x2C:
				opBIT(readWord(readDoubleWord(programCounter_ + 1)));
				break;
		//BMI (branch if minus)
			//Relative
			case 0x30:
				if (getNegativeFlag()) {
					pageCrossCheck(programCounter_+2, readWord(programCounter_+1));
					cycles++;
					programCounter_ += (int8_t) readWord(programCounter_+1);
				}

				break;
		//BNE (branch if not equal)
			//Relative
			case 0xD0:
				if (!getZeroFlag()) {
					pageCrossCheck(programCounter_+2, readWord(programCounter_+1));
					cycles++;
					programCounter_ += (int8_t) readWord(programCounter_+1);
				}

				break;
		//BPL (branch if positive)
			//Relative
			case 0x10:
				if (!getNegativeFlag()) {
					pageCrossCheck(programCounter_+2, readWord(programCounter_+1));
					cycles++;
					programCounter_ += (int8_t) readWord(programCounter_+1);
				}

				break;
		//BRK (force interrupt)
			//Implied
			case 0x00:
				//Push PC and processor status to stack
				pushToStackDouble(programCounter_+2);
				pushToStack(status_ | 0x30);

				//Load interrupt vector into PC
				programCounter_ = readDoubleWord(0xFFFE);

				setInterruptFlag();

				break;
		//BVC (branch if overflow clear)
			//Relative
			case 0x50:
				if (!getOverflowFlag()) {
					pageCrossCheck(programCounter_+2, readWord(programCounter_+1));
					cycles++;
					programCounter_ += (int8_t) readWord(programCounter_+1);
				}

				break;
		//BVS (branch if overflow set)
			//Relative
			case 0x70:
				if (getOverflowFlag()) {
					pageCrossCheck(programCounter_+2, readWord(programCounter_+1));
					cycles++;
					programCounter_ += (int8_t) readWord(programCounter_+1);
				}

				break;
 		//CLC (clear Carry flag)
			case 0x18:
				clearCarryFlag();
				break;
		//CLD (clear decimal mode)
			case 0xD8:
				clearDecimalFlag();
				break;
		//CLI (clear interrupt disable)
			case 0x58:
				clearInterruptFlag();
				break;
		//CLV (clear overflow flag)
			case 0xB8:
				clearOverflowFlag();
				break;
		//CMP (compare)
			//Immediate
			case 0xC9:
				opCMP(readWord(programCounter_ + 1));
				break;
			//Zero page
			case 0xC5:
				opCMP(readZeroPage(readWord(programCounter_ + 1)));
				break;
			//Zero page,X
			case 0xD5:
				opCMP(readZeroPage(readWord(programCounter_ + 1) + x_));
				break;
			//Absolute
			case 0xCD:
				opCMP(readWord(readDoubleWord(programCounter_ + 1)));
				break;
			//Absolute,X
			case 0xDD:
				pageCrossCheck(readDoubleWord(programCounter_+1), x_);

				opCMP(readWord(readDoubleWord(programCounter_ + 1) + x_));
				break;
			//Absolute,Y
			case 0xD9:
				pageCrossCheck(readDoubleWord(programCounter_+1), y_);

				opCMP(readWord(readDoubleWord(programCounter_ + 1) + y_));
				break;
			//Index,X
			case 0xC1:
				opCMP(readWord(readDoubleWord(readWord(programCounter_ + 1) + x_)));
				break;
			//Index,Y
			case 0xD1:
				pageCrossCheck(readDoubleWord(readWord(programCounter_ + 1)), y_);

				opCMP(readWord(readDoubleWord(readWord(programCounter_ + 1)) + y_));
				break;
		//CPX (compare x register)
			//Immediate
			case 0xE0:
				opCPX(readWord(programCounter_ + 1));
				break;
			//Zero page
			case 0xE4:
				opCPX(readZeroPage(readWord(programCounter_ + 1)));
				break;
			//Absolute
			case 0xEC:
				opCPX(readWord(readDoubleWord(programCounter_ + 1)));
				break;
		//CPY (compare y register)
			//Immediate
			case 0xC0:
				opCPY(readWord(programCounter_ + 1));
				break;
			//Zero page
			case 0xC4:
				opCPY(readZeroPage(readWord(programCounter_ + 1)));
				break;
			//Absolute
			case 0xCC:
				opCPY(readWord(readDoubleWord(programCounter_ + 1)));
				break;
 		//DEC (decrement memory)
			//Zero Page
			case 0xC6:
				opDEC(readWord(programCounter_+1));
				break;
			//Zero Page,X
			case 0xD6:
				opDEC((readWord(programCounter_+1) + x_) & 0xFF);
				break;
			//Absolute
			case 0xCE:
				opDEC(readDoubleWord(programCounter_+1));
				break;
			//Absolute, X
			case 0xDE:
				opDEC(readDoubleWord(programCounter_+1) + x_);
				break;
 		//DEX (decrement x register)
			//Implied
			case 0xCA:
				x_--;

				if (x_ == 0)
					setZeroFlag();
				else
					clearZeroFlag();

				if (x_ & 0x80)
					setNegativeFlag();
				else
					clearNegativeFlag();

				break;
		//DEY (decrement y register)
			//Implied
			case 0x88:
				y_--;

				if (y_ == 0)
					setZeroFlag();
				else
					clearZeroFlag();

				if (y_ & 0x80)
					setNegativeFlag();
				else
					clearNegativeFlag();

				break;
 		//EOR (exclusive or)
			//Immediate
			case 0x49:
				opEOR(readWord(programCounter_+1));
				break;
			//Zero page
			case 0x45:
				opEOR(readZeroPage(readWord(programCounter_+1)));
				break;
			//Zero page x
			case 0x55:
				opEOR(readZeroPage(readWord(programCounter_+1) + x_));
				break;
			//Absolute
			case 0x4D:
				opEOR(readWord(readDoubleWord(programCounter_+1)));
				break;
			//Absolute X
			case 0x5D:
				pageCrossCheck(readDoubleWord(programCounter_+1), x_);

				opEOR(readWord(readDoubleWord(programCounter_+1) + x_));
				break;
			//Absolute Y
			case 0x59:
				pageCrossCheck(readDoubleWord(programCounter_+1), y_);

				opEOR(readWord(readDoubleWord(programCounter_+1) + y_));
				break;
			//Indirect x
			case 0x41:
				opEOR(readWord(readZeroPageDouble(readWord(programCounter_+1) + x_)));
				break;
			//Indirect y
			case 0x51:
				pageCrossCheck(readZeroPageDouble(readWord(programCounter_+1)), y_);

				opEOR(readWord(readZeroPageDouble(readWord(programCounter_+1)) + y_));
				break;
		//INC (increment memory)
			//Zero Page
			case 0xE6:
				opINC(readWord(programCounter_+1));
				break;
			//Zero Page,X
			case 0xF6:
				opINC((readWord(programCounter_+1) + x_) & 0xFF);
				break;
			//Absolute
			case 0xEE:
				opINC(readDoubleWord(programCounter_+1));
				break;
			//Absolute, X
			case 0xFE:
				opINC(readDoubleWord(programCounter_+1) + x_);
				break;
 		//INX (increment x register)
			//Implied
			case 0xE8:
				x_++;

				if (x_ == 0)
					setZeroFlag();
				else
					clearZeroFlag();

				if (x_ & 0x80)
					setNegativeFlag();
				else
					clearNegativeFlag();

				break;
		//INY (increment y register)
			//Implied
			case 0xC8:
				y_++;

				if (y_ == 0)
					setZeroFlag();
				else
					clearZeroFlag();

				if (y_ & 0x80)
					setNegativeFlag();
				else
					clearNegativeFlag();

				break;
		//JMP (jump)
			//Absolute
			case 0x4C:
				programCounter_ = readDoubleWord(programCounter_ + 1);
				break;
			//Indirect
			case 0x6C:
				programCounter_ = readDoubleWord(readDoubleWord(programCounter_ + 1));
				break;
		//JSR
			//Absolute
			case 0x20:
				pushToStackDouble(programCounter_+2);
				programCounter_ = readDoubleWord(programCounter_ + 1);
				break;
 		//LDA (load accumulator)
			//Immediate
			case 0xA9:
				opLDA(readWord(programCounter_+1));
				break;
			//Zero page
			case 0xA5:
				opLDA(readZeroPage(readWord(programCounter_+1)));
				break;
			//Zero page x
			case 0xB5:
				opLDA(readZeroPage(readWord(programCounter_+1) + x_));
				break;
			//Absolute
			case 0xAD:
				opLDA(readWord(readDoubleWord(programCounter_+1)));
				break;
			//Absolute X
			case 0xBD:
				pageCrossCheck(readDoubleWord(programCounter_+1), x_);

				opLDA(readWord(readDoubleWord(programCounter_+1) + x_));
				break;
			//Absolute Y
			case 0xB9:
				pageCrossCheck(readDoubleWord(programCounter_+1), y_);

				opLDA(readWord(readDoubleWord(programCounter_+1) + y_));
				break;
			//Indirect x
			case 0xA1:
				opLDA(readWord(readZeroPageDouble(readWord(programCounter_+1) + x_)));
				break;
			//Indirect y
			case 0xB1:
				pageCrossCheck(readZeroPageDouble(readWord(programCounter_+1)), y_);

				opLDA(readWord(readZeroPageDouble(readWord(programCounter_+1)) + y_));
				break;
		//LDX (load x register)
			//Immediate
			case 0xA2:
				opLDX(readWord(programCounter_ + 1));
				break;
			//Zero Page
			case 0xA6:
				opLDX(readZeroPage(readWord(programCounter_+1)));
				break;
			//Zero Page,Y
			case 0xB6:
				opLDX(readZeroPage(readWord(programCounter_+1) + y_));
				break;
			//Absolute
			case 0xAE:
				opLDX(readWord(readDoubleWord(programCounter_+1)));
				break;
			//Absolute, Y
			case 0xBE:
				pageCrossCheck(readDoubleWord(programCounter_+1), y_);

				opLDX(readWord(readDoubleWord(programCounter_+1) + y_));
				break;
 		//LDY (load y register)
			//Immediate
			case 0xA0:
				opLDY(readWord(programCounter_ + 1));
				break;
			//Zero Page
			case 0xA4:
				opLDY(readZeroPage(readWord(programCounter_+1)));
				break;
			//Zero Page,X
			case 0xB4:
				opLDY(readZeroPage(readWord(programCounter_+1) + x_));
				break;
			//Absolute
			case 0xAC:
				opLDY(readWord(readDoubleWord(programCounter_+1)));
				break;
			//Absolute, X
			case 0xBC:
				pageCrossCheck(readDoubleWord(programCounter_+1), x_);

				opLDY(readWord(readDoubleWord(programCounter_+1) + x_));
				break;
 		//LSR (logical shift right)
			//Accumulator
			case 0x4A:
				//Grab the high bit for the carry flag
				if (acc_ & 0x01)
					setCarryFlag();
				else
					clearCarryFlag();

				acc_ = (acc_ >> 1) & 0x7F;

				//Sets zero flag if result is zero
				if (acc_ == 0)
					setZeroFlag();
				else
					clearZeroFlag();

				//Sets negative flag is result is negative
				if (acc_ & 0x80)
					setNegativeFlag();
				else
					clearNegativeFlag();
				break;
			//Zero Page
			case 0x46:
				opLSR(readWord(programCounter_+1));
				break;
			//Zero Page,X
			case 0x56:
				opLSR((readWord(programCounter_+1) + x_) & 0xFF);
				break;
			//Absolute
			case 0x4E:
				opLSR(readDoubleWord(programCounter_+1));
				break;
			//Absolute,X
			case 0x5E:
				opLSR(readDoubleWord(programCounter_+1) + x_);
				break;
		//NOP (no operation)
			case 0xEA:
				break;
		//ORA (logical inclusive or)
			//Immediate
			case 0x09:
				opORA(readWord(programCounter_+1));
				break;
			//Zero page
			case 0x05:
				opORA(readZeroPage(readWord(programCounter_+1)));
				break;
			//Zero page x
			case 0x15:
				opORA(readZeroPage(readWord(programCounter_+1) + x_));
				break;
			//Absolute
			case 0x0D:
				opORA(readWord(readDoubleWord(programCounter_+1)));
				break;
			//Absolute X
			case 0x1D:
				pageCrossCheck(readDoubleWord(programCounter_+1), x_);

				opORA(readWord(readDoubleWord(programCounter_+1) + x_));
				break;
			//Absolute Y
			case 0x19:
				pageCrossCheck(readDoubleWord(programCounter_+1), y_);

				opORA(readWord(readDoubleWord(programCounter_+1) + y_));
				break;
			//Indirect x
			case 0x01:
				opORA(readWord(readZeroPageDouble(readWord(programCounter_+1) + x_)));
				break;
			//Indirect y
			case 0x11:
				pageCrossCheck(readZeroPageDouble(readWord(programCounter_+1)), y_);

				opORA(readWord(readZeroPageDouble(readWord(programCounter_+1)) + y_));
				break;
		//PHA (push accumulator)
			//Implied
			case 0x48:
				pushToStack(acc_);
				break;
		//PHP (push processor status)
			//Implied
			case 0x08:
				pushToStack(status_ | 0x30);
				break;
		//PLA (pull accumulator)
			case 0x68:
				acc_ = popFromStack();

				if (acc_ == 0)
					setZeroFlag();
				else
					clearZeroFlag();

				if (acc_ & 0x80)
					setNegativeFlag();
				else
					clearNegativeFlag();

				break;
		//PLP (pull processor status)
			case 0x28:
				status_ = popFromStack();
				break;
		//ROL (rotate left)
			//Accumulator
			case 0x2A:
				if (acc_ & 0x80) {
					acc_ <<= 1;

					if (getCarryFlag())
						acc_ |= 0x01;
					else
						acc_ &= 0xFE;

					setCarryFlag();
				}
				else {
					acc_ <<= 1;

					if (getCarryFlag())
						acc_ |= 0x01;
					else
						acc_ &= 0xFE;

					clearCarryFlag();
				}

				if (acc_ == 0)
					setZeroFlag();
				else
					clearZeroFlag();

				if (acc_ & 0x80)
					setNegativeFlag();
				else
					clearNegativeFlag();

				break;
			//Zero page
			case 0x26:
				opROL(readWord(programCounter_ + 1));
				break;
			//Zero page,X
			case 0x36:
				opROL((readWord(programCounter_ + 1) + x_) & 0xFF);
				break;
			//Absolute
			case 0x2E:
				opROL(readDoubleWord(programCounter_ + 1));
				break;
			//Absolute,X
			case 0x3E:
				opROL(readDoubleWord(programCounter_ + 1) + x_);
				break;
 		//ROR (rotate right)
			//Accumulator
			case 0x6A:
				if (acc_ & 0x01) {
					acc_ >>= 1;

					if (getCarryFlag())
						acc_ |= 0x80;
					else
						acc_ &= 0x7F;

					setCarryFlag();
				}
				else {
					acc_ >>= 1;

					if (getCarryFlag())
						acc_ |= 0x80;
					else
						acc_ &= 0x7F;

					clearCarryFlag();
				}

				if (acc_ == 0)
					setZeroFlag();
				else
					clearZeroFlag();

				if (acc_ & 0x80)
					setNegativeFlag();
				else
					clearNegativeFlag();

				break;
			//Zero page
			case 0x66:
				opROR(readWord(programCounter_ + 1));
				break;
			//Zero page,X
			case 0x76:
				opROR((readWord(programCounter_ + 1) + x_) & 0xFF);
				break;
			//Absolute
			case 0x6E:
				opROR(readDoubleWord(programCounter_ + 1));
				break;
			//Absolute,X
			case 0x7E:
				opROR(readDoubleWord(programCounter_ + 1) + x_);
				break;
 		//RTI (return from interrupt)
			//Implied
			case 0x40:
				status_ = popFromStack();
				programCounter_ = popFromStackDouble();
				break;
		//RTS (return from subroutine)
			//Implied
			case 0x60:
				programCounter_ = popFromStackDouble();
				break;
		//SBC (subtract memory from accumulator with carry)
			//Immediate
			case 0xE9:
				opSBC(readWord(programCounter_+1));
				break;
			//Zero page
			case 0xE5:
				opSBC(readZeroPage(readWord(programCounter_+1)));
				break;
			//Zero page,X
			case 0xF5:
				opSBC(readZeroPage(readWord(programCounter_+1)+x_));
				break;
			//Absolute
			case 0xED:
				opSBC(readWord(readDoubleWord(programCounter_+1)));
				break;
			//Absolute,X
			case 0xFD:
				pageCrossCheck(readDoubleWord(programCounter_+1), x_);

				opSBC(readWord(readDoubleWord(programCounter_+1) + x_));
				break;
			//Absolute, Y
			case 0xF9:
				pageCrossCheck(readDoubleWord(programCounter_+1), y_);

				opSBC(readWord(readDoubleWord(programCounter_+1) + y_));
				break;
			//(Indirect,X)
			case 0xE1:
				opSBC(readWord(readZeroPageDouble(readWord(programCounter_+1) + x_)));
				break;
			//(Indirect),Y
			case 0xF1:
				pageCrossCheck(readZeroPageDouble(readWord(programCounter_+1)), y_);

				opSBC(readWord(readZeroPageDouble(readWord(programCounter_+1)) + y_));
				break;
		//SEC (set carry flag)
			//Implied
			case 0x38:
				setCarryFlag();
				break;
		//SED (set decimal flag)
			//Implied
			case 0xF8: 
				setDecimalFlag();
				break;
		//SEI (set interrupt disable)
			//Implied
			case 0x78:
				setInterruptFlag();
				break;
		//STA (store accumulator)
			//Zero page
			case 0x85: 
				writeZeroPage(readWord(programCounter_ + 1), acc_);
				break;
			//Zero page, x
			case 0x95: 
				writeZeroPage(readWord(programCounter_ + 1) + x_, acc_);
				break;
			//Absolute
			case 0x8D:
				writeWord(readDoubleWord(programCounter_ + 1), acc_);
				break;
			//Absolute,X
			case 0x9D:
				writeWord(readDoubleWord(programCounter_ + 1) + x_, acc_);
				break;
			//Absolute,Y
			case 0x99:
				writeWord(readDoubleWord(programCounter_ + 1) + y_, acc_);
				break;
 			//Index,X
			case 0x81:
				writeWord(readZeroPageDouble(readWord(programCounter_ + 1) + x_), acc_);
				break;
			//Index,X
			case 0x91:
				writeWord(readZeroPageDouble(readWord(programCounter_ + 1)) + y_, acc_);
				break;
 		//STX (store x register)
			//Zero page
			case 0x86:
				writeZeroPage(readWord(programCounter_ + 1), x_);
				break;
 			//Zero page,y
			case 0x96:
				writeZeroPage(readWord(programCounter_ + 1) + y_, x_);
				break;
 			//Absolute
			case 0x8E:
				writeWord(readDoubleWord(programCounter_ + 1), x_);
				break;
 		//STY (store y register)
			//Zero page
			case 0x84:
				writeZeroPage(readWord(programCounter_ + 1), y_);
				break;
 			//Zero page,x
			case 0x94:
				writeZeroPage(readWord(programCounter_ + 1) + x_, y_);
				break;
 			//Absolute
			case 0x8C:
				writeWord(readDoubleWord(programCounter_ + 1), y_);
				break;
 		//TAX (transfer accumulator to X)
			//Implied
			case 0xAA:
				x_ = acc_;

				if (x_ == 0)
					setZeroFlag();
				else
					clearZeroFlag();

				if (x_ & 0x80)
					setNegativeFlag();
				else
					clearNegativeFlag();

				break;
		//TAY (transfer accumulator to Y)
			//Implied
			case 0xA8:
				y_ = acc_;

				if (y_ == 0)
					setZeroFlag();
				else
					clearZeroFlag();

				if (y_ & 0x80)
					setNegativeFlag();
				else
					clearNegativeFlag();

				break;
 		//TSX (transfer stack pointer to x)
			//Implied
			case 0xBA:
				x_ = stackPointer_;

				if (x_ == 0)
					setZeroFlag();
				else
					clearZeroFlag();

				if (x_ & 0x80)
					setNegativeFlag();
				else
					clearNegativeFlag();

				break;
		//TXA (transfer x to accumulator)
			case 0x8A:
				acc_ = x_;

				if (acc_ == 0)
					setZeroFlag();
				else
					clearZeroFlag();

				if (acc_ & 0x80)
					setNegativeFlag();
				else
					clearNegativeFlag();

				break;
		//TXS (transfer x to stack pointer)
			case 0x9A:
				stackPointer_ = x_;
				break;
		//TYA (transfer y to accumulator)
			case 0x98:
				acc_ = y_;

				if (acc_ == 0)
					setZeroFlag();
				else
					clearZeroFlag();

				if (acc_ & 0x80)
					setNegativeFlag();
				else
					clearNegativeFlag();

				break;
		//DEFAULT FINALLY FUCK THAT TOOK A WHOLE DAY OF NOTHING BUT CODING AND I HAVEN'T TESTED ANYTHING YET
			default:
				throw(InvalidOpCodeException(opCode));
	 		}

		programCounter_ += ops[opCode].bytes;

		cycles += ops[opCode].cycles;
	}

	//The following setters are purely for debug purposes
	//and shouldn't be used in normal operation
	uint8_t getX() {
		return x_;
	}
	void setX(uint8_t x) {
		x_ = x;
	}

	uint8_t getY() {
		return y_;
	}
	void setY(uint8_t y) {
		y_ = y;
	}

	uint8_t getAcc() {
		return acc_;
	}
	void setAcc(uint8_t acc) {
		acc_ = acc;
	}

	uint8_t getStatus() {
		return status_;
	}
	void setStatus(uint8_t status) {
		status_ = status;
	}

	uint8_t getStackPointer() {
		return stackPointer_;
	}
	void setStackPointer(uint8_t stackPointer) {
		stackPointer_ = stackPointer;
	}

	uint16_t getProgramCounter() {
		return programCounter_;
	}
	void setProgramCounter(uint16_t programCounter) {
		programCounter_ = programCounter;
	}

	Operation getCurrentOp() {
		return ops[readWord(programCounter_)];
	}
};