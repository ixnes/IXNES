#include "6502.h"
#include "Console.h"

void CPU::setCarryFlag() {
	status |= 0x01;
}

void CPU::clearCarryFlag() {
	status &= 0xFE;
}

bool CPU::getCarryFlag() {
	return status & 0x01;
}

void CPU::setZeroFlag() {
	status |= 0x02;
}

void CPU::clearZeroFlag() {
	status &= 0xFD;
}

bool CPU::getZeroFlag() {
	return status & 0x02;
}

void CPU::setInterruptFlag() {
	status |= 0x04;
}

void CPU::clearInterruptFlag() {
	status &= 0xFB;
}

bool CPU::getInterruptFlag() {
	return status & 0x04;
}

void CPU::setDecimalFlag() {
	status |= 0x08;
}

void CPU::clearDecimalFlag() {
	status &= 0xF7;
}

bool CPU::getDecimalFlag() {
	return status & 0x08;
}

void CPU::setOverflowFlag() {
	status |= 0x40;
}

void CPU::clearOverflowFlag() {
	status &= 0xBF;
}

bool CPU::getOverflowFlag() {
	return status & 0x40;
}

void CPU::setNegativeFlag() {
	status |= 0x80;
}

void CPU::clearNegativeFlag() {
	status &= 0x7F;
}


bool CPU::getNegativeFlag() {
	return status & 0x80;
}

//called at the end of instruction execution to prepare
//cpu to accept next instruction
void CPU::exitInstruction() {
	dataTemp = -1;
	inInstruction = false;
	cycleCounter = 0;
}

//This function is responsible for emulating the
//sequence of loading the data for an operation
//
//Certain operations can skip a cycle under certain
//addressing modes, if carrySkip is true, the function
//which called is for one of those operations
int16_t CPU::readData(bool carrySkip) {
	if (currentMode == AddressMode::Immediate) {
		uint8_t ret = console->cpuRead(programCounter);
		programCounter++;
		return ret;
	}
	else if (currentMode == AddressMode::ZeroPage) {
		if (cycleCounter == 0) {
			//Retrieve ADL
			addressTemp = 0x0000 | console->cpuRead(programCounter);
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			//Retrieve data
			uint8_t ret = console->cpuRead(addressTemp);
			cycleCounter = 0;
			return ret;
		}
	}
	else if (currentMode == AddressMode::ZeroPageX) {
		if (cycleCounter == 0) {
			//Retrieve BAL
			addressTemp = 0x0000 | console->cpuRead(programCounter);
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			//Dummy fetch
			console->cpuRead(addressTemp);
			cycleCounter++;
		}
		else if (cycleCounter == 2) {
			//Retrieve data
			addressTemp = 0x00FF & (addressTemp + x);
			uint8_t ret = console->cpuRead(addressTemp);
			cycleCounter = 0;
			return ret;
		}
	}
	else if (currentMode == AddressMode::ZeroPageY) {
		if (cycleCounter == 0) {
			//Retrieve BAL
			addressTemp = 0x0000 | console->cpuRead(programCounter);
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			//Dummy fetch
			console->cpuRead(addressTemp);
			cycleCounter++;
		}
		else if (cycleCounter == 2) {
			//Retrieve data
			addressTemp = 0x00FF & (addressTemp + y);
			uint8_t ret = console->cpuRead(addressTemp);
			cycleCounter = 0;
			return ret;
		}
	}
	else if (currentMode == AddressMode::Absolute) {
		if (cycleCounter == 0) {
			//Retrieve ADL
			addressTemp = 0x0000 | console->cpuRead(programCounter);
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			//Retrieve ADH
			addressTemp |= console->cpuRead(programCounter) << 8;
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 2) {
			//Retrieve data
			uint8_t ret = console->cpuRead(addressTemp);
			cycleCounter = 0;
			return ret;
		}
	} 
	else if (currentMode == AddressMode::AbsoluteX) {
		if (cycleCounter == 0) {
			//Retrieve BAL
			addressTemp = 0x0000 | console->cpuRead(programCounter);
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			//Retrieve BAH
			addressTemp |= console->cpuRead(programCounter) << 8;
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 2) {
			//Retrieve data (1st)
			uint16_t addressNoCarry = ((addressTemp + x) & 0x00FF ) | (addressTemp & 0xFF00);
			uint8_t ret = console->cpuRead(addressNoCarry);
			if ( (addressTemp + x == addressNoCarry) && carrySkip ) {
				addressTemp = addressNoCarry;	
				cycleCounter = 0;
				return ret;
			}
			else {
				cycleCounter++;
			}
		}
		else if (cycleCounter == 3) {
			//Retrieve data (2nd)
			addressTemp += x;
			uint8_t ret = console->cpuRead(addressTemp);
			cycleCounter = 0;
			return ret;
		}
	}
	else if (currentMode == AddressMode::AbsoluteY) {
		if (cycleCounter == 0) {
			//Retrieve BAL
			addressTemp = 0x0000 | console->cpuRead(programCounter);
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			//Retrieve BAH
			addressTemp |= console->cpuRead(programCounter) << 8;
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 2) {
			//Retrieve data (1st)
			uint16_t addressNoCarry = ((addressTemp + y) & 0x00FF ) | (addressTemp & 0xFF00);
			uint8_t ret = console->cpuRead(addressNoCarry);
			if ( (addressTemp + y == addressNoCarry) && carrySkip ) {	
				addressTemp = addressNoCarry;
				cycleCounter = 0;
				return ret;
			}
			else {
				cycleCounter++;
			}
		}
		else if (cycleCounter == 3) {
			//Retrieve data (2nd)
			addressTemp += y;
			uint8_t ret = console->cpuRead(addressTemp);
			cycleCounter = 0;
			return ret;
		}
	}
	else if (currentMode == AddressMode::IndirectX) {
		if (cycleCounter == 0) {
			//Retrieve BAL
			addressTempInd = 0x0000 | console->cpuRead(programCounter);
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			//Dummy read
			console->cpuRead(addressTempInd);
			cycleCounter++;
		}
		else if (cycleCounter == 2) {
			//Retrieve ADL
			addressTemp = 0x0000 | console->cpuRead((addressTempInd + x) & 0x00FF);
			cycleCounter++;
		}
		else if (cycleCounter == 3) {
			//Retrieve ADH
			addressTemp |= console->cpuRead((addressTempInd + x + 1) & 0x00FF) << 8;
			cycleCounter++;
		}
		else if (cycleCounter == 4) {
			//Rertrieve data
			uint8_t ret = console->cpuRead(addressTemp);
			cycleCounter = 0;
			return ret;
		}

	}
	else if (currentMode == AddressMode::IndirectY) {
		if (cycleCounter == 0) {
			//Retrieve IAL
			addressTempInd = console->cpuRead(programCounter);
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			//Retrieve BAL
			addressTemp = 0x0000 | console->cpuRead(addressTempInd);
			cycleCounter++;
		}
		else if (cycleCounter == 2) {
			//Retrieve BAH
			addressTemp |= console->cpuRead( 0x00FF & (addressTempInd + 1)) << 8;
			cycleCounter++;
		}
		else if (cycleCounter == 3) {
			//Retrieve data (1st)
			uint16_t addressNoCarry = ((addressTemp + y) & 0x00FF ) | (addressTemp & 0xFF00);
			uint8_t ret = console->cpuRead(addressNoCarry);
			if ( (addressTemp + y == addressNoCarry) && carrySkip ) {	
				cycleCounter = 0;
				return ret;
			}
			else {
				cycleCounter++;
			}
		}
		else if (cycleCounter == 4) {
			addressTemp += y;
			uint8_t ret = console->cpuRead(addressTemp);
			cycleCounter = 0;
			return ret;
		}
	}
	return -1;
}

//This function is responsible for emulating the
//sequence of writing the data for an operation
//
//'data' is the data to be written in the effective address
int16_t CPU::writeData(uint8_t data) {
	if (currentMode == AddressMode::ZeroPage) {
		if (cycleCounter == 0) {
			//Retrieve ADL
			addressTemp = 0x0000 | console->cpuRead(programCounter);
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			//Write data
			console->cpuWrite(addressTemp, data);
			cycleCounter = 0;
			return 0;
		}
	}
	else if (currentMode == AddressMode::ZeroPageX) {
		if (cycleCounter == 0) {
			//Retrieve BAL
			addressTemp = 0x0000 | console->cpuRead(programCounter);
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			//Dummy fetch
			console->cpuRead(addressTemp);
			cycleCounter++;
		}
		else if (cycleCounter == 2) {
			//Write data
			console->cpuWrite(0x00FF & (addressTemp + x), data);
			cycleCounter = 0;
			return 0;
		}
	}
	else if (currentMode == AddressMode::ZeroPageY) {
		if (cycleCounter == 0) {
			//Retrieve BAL
			addressTemp = 0x0000 | console->cpuRead(programCounter);
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			//Dummy fetch
			console->cpuRead(addressTemp);
			cycleCounter++;
		}
		else if (cycleCounter == 2) {
			//Write data
			console->cpuWrite(0x00FF & (addressTemp + y), data);
			cycleCounter = 0;
			return 0;
		}
	}
	else if (currentMode == AddressMode::Absolute) {
		if (cycleCounter == 0) {
			//Retrieve ADL
			addressTemp = 0x0000 | console->cpuRead(programCounter);
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			//Retrieve ADH
			addressTemp |= console->cpuRead(programCounter) << 8;
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 2) {
			//Write data
			console->cpuWrite(addressTemp, data);
			cycleCounter = 0;
			return 0;
		}
	} 
	else if (currentMode == AddressMode::AbsoluteX) {
		if (cycleCounter == 0) {
			//Retrieve BAL
			addressTemp = 0x0000 | console->cpuRead(programCounter);
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			//Retrieve BAH
			addressTemp |= console->cpuRead(programCounter) << 8;
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 2) {
			//Dummy fetch
			console->cpuRead( ( (addressTemp + x) & 0x00FF ) | (addressTemp & 0xFF00) );
			cycleCounter++;
		}
		else if (cycleCounter == 3) {
			//Write data
			console->cpuWrite(addressTemp + x, data);
			cycleCounter = 0;
			return 0;
		}
	}
	else if (currentMode == AddressMode::AbsoluteY) {
		if (cycleCounter == 0) {
			//Retrieve BAL
			addressTemp = 0x0000 | console->cpuRead(programCounter);
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			//Retrieve BAH
			addressTemp |= console->cpuRead(programCounter) << 8;
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 2) {
			//Dummy fetch
			console->cpuRead( ( (addressTemp + y) & 0x00FF ) | (addressTemp & 0xFF00) );
			cycleCounter++;
		}
		else if (cycleCounter == 3) {
			//Write data
			console->cpuWrite(addressTemp + y, data);
			cycleCounter = 0;
			return 0;
		}
	}
	else if (currentMode == AddressMode::IndirectX) {
		if (cycleCounter == 0) {
			//Retrieve BAL
			addressTempInd = 0x0000 | console->cpuRead(programCounter);
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			//Dummy read
			console->cpuRead(addressTempInd);
			cycleCounter++;
		}
		else if (cycleCounter == 2) {
			//Retrieve ADL
			addressTemp = 0x0000 | console->cpuRead((addressTempInd + x) & 0x00FF);
			cycleCounter++;
		}
		else if (cycleCounter == 3) {
			//Retrieve ADH
			addressTemp |= console->cpuRead((addressTempInd + x + 1) & 0x00FF) << 8;
			cycleCounter++;
		}
		else if (cycleCounter == 4) {
			//Write data
			console->cpuWrite(addressTemp, data);
			cycleCounter = 0;
			return 0;
		}
	}
	else if (currentMode == AddressMode::IndirectY) {
		if (cycleCounter == 0) {
			//Retrieve IAL
			addressTempInd = console->cpuRead(programCounter);
			programCounter++;
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			//Retrieve BAL
			addressTemp = 0x0000 | console->cpuRead(addressTempInd);
			cycleCounter++;
		}
		else if (cycleCounter == 2) {
			//Retrieve BAH
			addressTemp |= console->cpuRead( 0x00FF & (addressTempInd + 1)) << 8;
			cycleCounter++;
		}
		else if (cycleCounter == 3) {
			//Dummy read
			console->cpuRead( ( (addressTemp + y) & 0x00FF ) | (addressTemp & 0xFF00) );
			cycleCounter++;
		}
		else if (cycleCounter == 4) {
			//Write data
			console->cpuWrite(addressTemp + y, data);
			cycleCounter = 0;
			return 0;
		}
	}
	return -1;
}

//All branch instructions have the same cycle-by-cycle
//behaviour except, obviously, for the branch condition
//this is
void CPU::branch() {
	if (cycleCounter == 0) {
		//Get offset
		dataTemp = (int8_t)console->cpuRead(programCounter);
		cycleCounter++;
		programCounter++;
	}
	else if (cycleCounter == 1) {
		//If we don't have carry, update pc, do dummy opcode fetch and exit
		if (programCounter + dataTemp == ((programCounter + dataTemp) & 0x00FF) | (programCounter & 0xFF00)) {	
			programCounter += dataTemp;
			console->cpuRead(programCounter);
			cycleCounter = 0;
			exitInstruction();
		}
		//else, update pc w/o carry, do dummy opcode fetch, and continue
		else {
			programCounter = ((programCounter + dataTemp) & 0x00FF) | (programCounter & 0xFF00);
			console->cpuRead(programCounter);
			cycleCounter++;
		}
	}
	else if (cycleCounter == 2) {
		//finally add carry, do final dummy opcode fetch, and exit
		programCounter += 0x0100;
		console->cpuRead(programCounter);
		exitInstruction();
	}
}

void CPU::opADC() {
	int16_t data = readData(true);
	if (data != -1) {
		int16_t resultUnsigned = acc + ((uint8_t)data) + (getCarryFlag() ? 1 : 0);
		int16_t resultSigned   = ((int8_t)acc) + ((int8_t)data) + (getCarryFlag() ? 1 : 0);

		acc = resultUnsigned;

		//Sets carry flag if overflow in bit 7
		if (resultUnsigned > 255)
			setCarryFlag();
		else
			clearCarryFlag();	

		//Sets zero flag is result is zero
		if (acc == 0)
			setZeroFlag();
		else
			clearZeroFlag();
		

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

		//cleanup
		exitInstruction();
	}		
}

void CPU::opAND() {
	int16_t data = readData(true);
	if (data != -1) {
		acc &= data;

		//Sets zero flag if result is zero
		if (acc == 0)
			setZeroFlag();
		else
			clearZeroFlag();	

		//Sets negative flag is result is negative
		if (acc & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		exitInstruction();
	}
}

void CPU::opASL() {
	if (currentMode == AddressMode::Accumulator) {
		//Grab the high bit for the carry flag
		if (acc & 0x80)
			setCarryFlag();
		else
			clearCarryFlag();
		acc <<= 1;

		//Sets zero flag if result is zero
		if (acc == 0)
			setZeroFlag();
		else
			clearZeroFlag();	

		//Sets negative flag is result is negative
		if (acc & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		exitInstruction();
		return;
	}

	if (dataTemp == -1) {
		dataTemp = readData(false);
	}
	else {
		if (cycleCounter == 0) {
			console->cpuWrite(addressTemp, (uint8_t) dataTemp);
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			//Grab the high bit for the carry flag
			if (dataTemp & 0x80)
				setCarryFlag();
			else
				clearCarryFlag();
			dataTemp <<= 1;

			//Sets zero flag if result is zero
			if (dataTemp == 0)
				setZeroFlag();
			else
				clearZeroFlag();

			//Sets negative flag is result is negative
			if (dataTemp & 0x80)
				setNegativeFlag();
			else
				clearNegativeFlag();


			console->cpuWrite(addressTemp, (uint8_t) dataTemp);
			exitInstruction();
		}
	}
}

void CPU::opBCC() {
	//If the condition is met, branch
	if (!getCarryFlag()) {
		branch();			
	}
	//Else bail
	else {
		programCounter++;
		exitInstruction();
		return;
	}
}

void CPU::opBCS() {
	//If the condition is met, branch
	if (getCarryFlag()) {
		branch();			
	}
	//Else bail
	else {
		programCounter++;
		exitInstruction();
		return;
	}
}

void CPU::opBEQ() {
	//If the condition is met, branch
	if (getZeroFlag()) {
		branch();			
	}
	//Else bail
	else {
		programCounter++;
		exitInstruction();
		return;
	}
}

void CPU::opBIT() {
	int16_t data = readData(true);
	if (data != -1) {
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

		if ((acc & data) == 0)
			setZeroFlag();
		else
			clearZeroFlag();

		exitInstruction();
	}
}

void CPU::opBMI() {
	//If the condition is met, branch
	if (getNegativeFlag()) {
		branch();			
	}
	//Else bail
	else {
		programCounter++;
		exitInstruction();
		return;
	}
}

void CPU::opBNE() {
	//If the condition is met, branch
	if (!getZeroFlag()) {
		branch();			
	}
	//Else bail
	else {
		programCounter++;
		exitInstruction();
		return;
	}
}

void CPU::opBPL() {
	//If the condition is met, branch
	if (!getNegativeFlag()) {
		branch();			
	}
	//Else bail
	else {
		programCounter++;
		exitInstruction();
		return;
	}
}

void CPU::opBRK() {
	if (cycleCounter == 0) {
		//Dummy read
		console->cpuRead(programCounter);
		programCounter++;
		cycleCounter++;
	}
	else if (cycleCounter == 1) {
		//Push PCH to stack
		console->cpuWrite(0x0100 + stackPointer, (programCounter & 0xFF00) >> 8);
		stackPointer--;
		cycleCounter++;
	}
	else if (cycleCounter == 2) {
		//Push PCL to stack
		console->cpuWrite(0x0100 + stackPointer, programCounter & 0x00FF);
		stackPointer--;
		cycleCounter++;
	}
	else if (cycleCounter == 3) {
		//Push P to stack
		console->cpuWrite(0x0100 + stackPointer, status | 0x30);
		stackPointer--;
		setInterruptFlag();
		cycleCounter++;
	}
	else if (cycleCounter == 4) {
		//fetch ADL
		programCounter = 0x0000 | console->cpuRead(0xFFFE);
		cycleCounter++;
	}
	else if (cycleCounter == 5) {
		//fetch ADH
		programCounter |= console->cpuRead(0xFFFF) << 8;
		cycleCounter++;
		exitInstruction();
	}
}

void CPU::opBVC() {
	//If the condition is met, branch
	if (!getOverflowFlag()) {
		branch();			
	}
	//Else bail
	else {
		programCounter++;
		exitInstruction();
		return;
	}
}

void CPU::opBVS() {
	//If the condition is met, branch
	if (getOverflowFlag()) {
		branch();			
	}
	//Else bail
	else {
		programCounter++;
		exitInstruction();
		return;
	}
}

void CPU::opCLC() {
	clearCarryFlag();
	exitInstruction();
}

void CPU::opCLD() {
	clearDecimalFlag();
	exitInstruction();
}

void CPU::opCLI() {
	clearInterruptFlag();
	exitInstruction();
}

void CPU::opCLV() {
	clearOverflowFlag();
	exitInstruction();
}

void CPU::opCMP() {
	int16_t data = readData(true);
	if (data != -1) {
		if (acc >= data)
			setCarryFlag();
		else
			clearCarryFlag();	

		if (acc == data)
			setZeroFlag();
		else
			clearZeroFlag();

		if ((acc-data) & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		exitInstruction();
	}
}

void CPU::opCPX() {
	int16_t data = readData(true);
	if (data != -1) {
		if (x >= data)
			setCarryFlag();
		else
			clearCarryFlag();	

		if (x == data)
			setZeroFlag();
		else
			clearZeroFlag();

		if ((x-data) & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		exitInstruction();
	}
}

void CPU::opCPY() {
	int16_t data = readData(true);
	if (data != -1) {
		if (y >= data)
			setCarryFlag();
		else
			clearCarryFlag();	

		if (y == data)
			setZeroFlag();
		else
			clearZeroFlag();

		if ((y-data) & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		exitInstruction();
	}
}

void CPU::opDEC() {
	if (dataTemp == -1) {
		dataTemp = readData(false);
	}
	else {
		if (cycleCounter == 0) {
			console->cpuWrite(addressTemp, (uint8_t) dataTemp);
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			uint8_t data = (0x00FF & dataTemp) - 1;

			if (data == 0)
				setZeroFlag();
			else
				clearZeroFlag();

			if (data & 0x80)
				setNegativeFlag();
			else
				clearNegativeFlag();

			console->cpuWrite(addressTemp, (uint8_t) data);
			exitInstruction();
		}
	}
}

void CPU::opDEX() {
	x--;

	if (x == 0)
		setZeroFlag();
	else
		clearZeroFlag();

	if (x & 0x80)
		setNegativeFlag();
	else
		clearNegativeFlag();

	exitInstruction();
}

void CPU::opDEY() {
	y--;

	if (y == 0)
		setZeroFlag();
	else
		clearZeroFlag();

	if (y & 0x80)
		setNegativeFlag();
	else
		clearNegativeFlag();

	exitInstruction();
}

void CPU::opEOR() {
	int16_t data = readData(true);
	if (data != -1) {
		acc ^= data;

		//Sets zero flag if result is zero
		if (acc == 0)
			setZeroFlag();
		else
			clearZeroFlag();	

		//Sets negative flag is result is negative
		if (acc & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		exitInstruction();
	}
}

void CPU::opINC() {
	if (dataTemp == -1) {
		dataTemp = readData(false);
	}
	else {
		if (cycleCounter == 0) {
			console->cpuWrite(addressTemp, (uint8_t) dataTemp);
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			uint8_t data = (0x00FF & dataTemp) + 1;

			if (data == 0)
				setZeroFlag();
			else
				clearZeroFlag();

			if (data & 0x80)
				setNegativeFlag();
			else
				clearNegativeFlag();

			console->cpuWrite(addressTemp, (uint8_t) data);
			exitInstruction();
		}
	}
}

void CPU::opINX() {
	x++;

	if (x == 0)
		setZeroFlag();
	else
		clearZeroFlag();

	if (x & 0x80)
		setNegativeFlag();
	else
		clearNegativeFlag();

	exitInstruction();
}

void CPU::opINY() {
	y++;

	if (y == 0)
		setZeroFlag();
	else
		clearZeroFlag();

	if (y & 0x80)
		setNegativeFlag();
	else
		clearNegativeFlag();

	exitInstruction();
}

void CPU::opJMP() {
	if (cycleCounter == 0) {
		//Retrieve address low
		addressTempInd = 0x0000 | console->cpuRead(programCounter);
		programCounter++;
		cycleCounter++;
	}
	else if (cycleCounter == 1) {
		//Retrieve address high
		addressTempInd |= console->cpuRead(programCounter) << 8;
		programCounter++;
		cycleCounter++;
		//if address mode is Indirect, continue and retrieve effective address
		//else just set program counter and exit
		if (currentMode == AddressMode::Absolute) {
			programCounter = addressTempInd;
			exitInstruction();
		}
	}
	else if (cycleCounter == 2) {
		//Retrieve address low
		addressTemp = 0x0000 | console->cpuRead(addressTempInd);
		cycleCounter++;
	}
	else if (cycleCounter == 3) {
		//Retrieve address high
		addressTemp |= console->cpuRead(addressTempInd + 1) << 8;
		programCounter = addressTemp;
		exitInstruction();
	}
}

void CPU::opJSR() {
	if (cycleCounter == 0) {
		//ADL fetch
		addressTemp = 0x0000 | console->cpuRead(programCounter);
		programCounter++;
		cycleCounter++;
	}
	else if (cycleCounter == 1) {
		//Dummy read
		console->cpuRead(0x0100 + stackPointer);
		cycleCounter++;
	}
	else if (cycleCounter == 2) {
		//Push PCH to stack
		console->cpuWrite(0x0100 + stackPointer, programCounter >> 8);
		stackPointer--;
		cycleCounter++;
	}
	else if (cycleCounter == 3) {
		//Push PCL to stack
		console->cpuWrite(0x0100 + stackPointer, programCounter & 0x00FF);
		stackPointer--;
		cycleCounter++;
	}
	else if (cycleCounter == 4) {
		//ADH fetch
		addressTemp |= console->cpuRead(programCounter) << 8;
		programCounter = addressTemp;
		exitInstruction();
	}
}

void CPU::opLDA() {
	int16_t data = readData(true);
	if (data != -1) {
		acc = data;

		//Sets zero flag if result is zero
		if (acc == 0)
			setZeroFlag();
		else
			clearZeroFlag();	

		//Sets negative flag if result is negative
		if (acc & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		exitInstruction();
	}
}

void CPU::opLDX() {
	int16_t data = readData(true);
	if (data != -1) {
		x = data;

		//Sets zero flag if result is zero
		if (x == 0)
			setZeroFlag();
		else
			clearZeroFlag();	

		//Sets negative flag if result is negative
		if (x & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		exitInstruction();
	}
}

void CPU::opLDY() {
	int16_t data = readData(true);
	if (data != -1) {
		y = data;

		//Sets zero flag if result is zero
		if (y == 0)
			setZeroFlag();
		else
			clearZeroFlag();	

		//Sets negative flag if result is negative
		if (y & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		exitInstruction();
	}
}

void CPU::opLSR() {
	if (currentMode == AddressMode::Accumulator) {
		//Grab the low bit for the carry flag
		if (acc & 0x01)
			setCarryFlag();
		else
			clearCarryFlag();

		acc = (acc >> 1) & 0x7F;

		//Sets zero flag if result is zero
		if (acc == 0)
			setZeroFlag();
		else
			clearZeroFlag();	

		//Sets negative flag is result is negative
		if (acc & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		exitInstruction();
		return;
	}

	if (dataTemp == -1) {
		dataTemp = readData(false);
	}
	else {
		if (cycleCounter == 0) {
			console->cpuWrite(addressTemp, (uint8_t) dataTemp);
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			uint8_t data = (0x00FF & dataTemp);

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

			console->cpuWrite(addressTemp, (uint8_t) data);
			exitInstruction();
		}
	}
}

void CPU::opNOP() {
	exitInstruction();
}

void CPU::opORA() {
	int16_t data = readData(true);
	if (data != -1) {
		acc |= data;

		//Sets zero flag if result is zero
		if (acc == 0)
			setZeroFlag();
		else
			clearZeroFlag();	

		//Sets negative flag is result is negative
		if (acc & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		exitInstruction();
	}
}

void CPU::opPHA() {
	if (cycleCounter == 0) {
		//Dummy read
		console->cpuRead(programCounter);
		cycleCounter++;
	}
	else if (cycleCounter == 1) {
		console->cpuWrite(0x0100 + stackPointer, acc);
		stackPointer--;
		exitInstruction();
	}
}

void CPU::opPHP() {
	if (cycleCounter == 0) {
		//Dummy read
		console->cpuRead(programCounter);
		cycleCounter++;
	}
	else if (cycleCounter == 1) {
		console->cpuWrite(0x0100 + stackPointer, status | 0x30);
		stackPointer--;
		exitInstruction();
	}
}

void CPU::opPLA() {
	if (cycleCounter == 0) {
		//Dummy read 1
		console->cpuRead(programCounter);
		cycleCounter++;
	}
	else if (cycleCounter == 1) {
		//Dummy read 2
		console->cpuRead(0x0100 + stackPointer);
		stackPointer++;
		cycleCounter++;
	}
	else if (cycleCounter == 2) {
		//Retrieve data
		acc = console->cpuRead(0x0100 + stackPointer);

		if (acc == 0)
			setZeroFlag();
		else
			clearZeroFlag();

		if (acc & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		exitInstruction();
	}
}

void CPU::opPLP() {
	if (cycleCounter == 0) {
		//Dummy read 1
		console->cpuRead(programCounter);
		cycleCounter++;
	}
	else if (cycleCounter == 1) {
		//Dummy read 2
		console->cpuRead( 0x0100 + stackPointer);
		stackPointer++;
		cycleCounter++;
	}
	else if (cycleCounter == 2) {
		//Retrieve data
		status = console->cpuRead(0x0100 + stackPointer);
		exitInstruction();
	}
}

void CPU::opROL() {
	if (currentMode == AddressMode::Accumulator) {
		if (acc & 0x80) {
				acc <<= 1;

				if (getCarryFlag())
					acc |= 0x01;
				else
					acc &= 0xFE;

				setCarryFlag();
			}
			else {
				acc <<= 1;

				if (getCarryFlag())
					acc |= 0x01;
				else
					acc &= 0xFE;

				clearCarryFlag();
			}

			if (acc == 0)
				setZeroFlag();
			else
				clearZeroFlag();

			if (acc & 0x80)
				setNegativeFlag();
			else
				clearNegativeFlag();

		exitInstruction();
		return;
	}

	if (dataTemp == -1) {
		dataTemp = readData(false);
	}
	else {
		if (cycleCounter == 0) {
			console->cpuWrite(addressTemp, (uint8_t) dataTemp);
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			uint8_t data = (0x00FF & dataTemp);

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

			console->cpuWrite(addressTemp, (uint8_t) data);
			exitInstruction();
		}
	}
}

void CPU::opROR() {
	if (currentMode == AddressMode::Accumulator) {
		if (acc & 0x01) {
			acc >>= 1;

			if (getCarryFlag())
				acc |= 0x80;
			else
				acc &= 0x7F;

			setCarryFlag();
		}
		else {
			acc >>= 1;

			if (getCarryFlag())
				acc |= 0x80;
			else
				acc &= 0x7F;

			clearCarryFlag();
		}

		if (acc == 0)
			setZeroFlag();
		else
			clearZeroFlag();

		if (acc & 0x80)
			setNegativeFlag();
		else
			clearNegativeFlag();

		exitInstruction();
		return;
	}

	if (dataTemp == -1) {
		dataTemp = readData(false);
	}
	else {
		if (cycleCounter == 0) {
			console->cpuWrite(addressTemp, (uint8_t) dataTemp);
			cycleCounter++;
		}
		else if (cycleCounter == 1) {
			uint8_t data = (0x00FF & dataTemp);

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


			console->cpuWrite(addressTemp, (uint8_t) data);
			exitInstruction();
		}
	}
}

void CPU::opRTI() {
	if (cycleCounter == 0) {
		//Dummy read 1
		console->cpuRead(programCounter);
		cycleCounter++;
	}
	else if (cycleCounter == 1) {
		//Dummy read 2
		console->cpuRead(0x0100 + stackPointer);
		cycleCounter++;
	}
	else if (cycleCounter == 2) {
		//Pull status from stack
		stackPointer++;
		status = console->cpuRead(0x0100 + stackPointer);
		cycleCounter++;
	}
	else if (cycleCounter == 3) {
		//Pull PCL from stack
		stackPointer++;
		addressTemp = 0x0000 | console->cpuRead(0x0100 + stackPointer);
		cycleCounter++;
	}
	else if (cycleCounter == 4) {
		//Pull PCH from stack
		stackPointer++;
		addressTemp |= console->cpuRead(0x0100 + stackPointer) << 8;
		programCounter = addressTemp;
		exitInstruction();
	}
}

void CPU::opRTS() {
	if (cycleCounter == 0) {
		//Dummy read 1
		console->cpuRead(programCounter);
		cycleCounter++;
	}
	else if (cycleCounter == 1) {
		//Dummy read 2
		console->cpuRead(0x0100 + stackPointer);
		cycleCounter++;
	}
	else if (cycleCounter == 2) {
		//Pull PCL from stack
		stackPointer++;
		addressTemp = 0x0000 | console->cpuRead(0x0100 + stackPointer);
		cycleCounter++;
	}
	else if (cycleCounter == 3) {
		//Pull PCH from stack
		stackPointer++;
		addressTemp |= console->cpuRead(0x0100 + stackPointer) << 8;
		programCounter = addressTemp;
		cycleCounter++;
	}
	else if (cycleCounter == 4) {
		//Dummy read 3
		console->cpuRead(programCounter);
		programCounter++;
		exitInstruction();
	}
}

void CPU::opSBC() {
	int16_t data = readData(true);
	if (data != -1) {
		//This ungodly ternary operator is because the 6502 does subtraction
		//in a weird way to support multibyte substraction
		//If you just want to subtract one byte you have to 
		//make sure the carry flag is set before you perform it
		int16_t resultUnsigned = acc  - data - (getCarryFlag() ? 0 : 1);
		int16_t resultSigned   = ((int8_t)acc) - ((int8_t)data) - (getCarryFlag() ? 0 : 1);

		acc = resultUnsigned;

		//Sets carry flag is the result is greater than or equal to zero
		if (resultUnsigned >= 0)
			setCarryFlag();
		else
			clearCarryFlag();

		//Sets zero flag is result is zero
		if (acc == 0)
			setZeroFlag();
		else
			clearZeroFlag();
		

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

		//cleanup
		exitInstruction();
	}		
}

void CPU::opSEC() {
	setCarryFlag();
	exitInstruction();
}

void CPU::opSED() {
	setDecimalFlag();
	exitInstruction();
}

void CPU::opSEI() {
	setInterruptFlag();
	exitInstruction();
}

void CPU::opSTA() {
	int16_t data = writeData(acc);
	if (data != -1) {
		exitInstruction();
	}
}

void CPU::opSTX() {
	int16_t data = writeData(x);
	if (data != -1) {
		exitInstruction();
	}
}

void CPU::opSTY() {
	int16_t data = writeData(y);
	if (data != -1) {
		exitInstruction();
	}
}

void CPU::opTAX() {
	x = acc;

	if (x == 0)
		setZeroFlag();
	else
		clearZeroFlag();

	if (x & 0x80)
		setNegativeFlag();
	else
		clearNegativeFlag();

	exitInstruction();
}

void CPU::opTAY() {
	y = acc;

	if (y == 0)
		setZeroFlag();
	else
		clearZeroFlag();

	if (y & 0x80)
		setNegativeFlag();
	else
		clearNegativeFlag();

	exitInstruction();
}

void CPU::opTSX() {
	x = stackPointer;

	if (x == 0)
		setZeroFlag();
	else
		clearZeroFlag();

	if (x & 0x80)
		setNegativeFlag();
	else
		clearNegativeFlag();

	exitInstruction();
}

void CPU::opTXA() {
	acc = x;

	if (acc == 0)
		setZeroFlag();
	else
		clearZeroFlag();

	if (acc & 0x80)
		setNegativeFlag();
	else
		clearNegativeFlag();

	exitInstruction();
}

void CPU::opTXS() {
	stackPointer = x;
	exitInstruction();
}

void CPU::opTYA() {
	acc = y;

	if (acc == 0)
		setZeroFlag();
	else
		clearZeroFlag();

	if (acc & 0x80)
		setNegativeFlag();
	else
		clearNegativeFlag();

	exitInstruction();
}

//Handles an unrecognized opcode being read
void CPU::opNotRecognized() {
	throw(InvalidOpCodeException(currentOp));
}

//Sets one entry in the lookup tables
void CPU::setLookup(int index, string name, void (CPU::*opFunc)(), AddressMode mode) {
	ops[index].name = name;
	ops[index].opFunc = opFunc;
	ops[index].mode = mode;
}

//Initializes the lookup tables
//Sets all unused codes to 0
void CPU::initializeLookups() {
	for (uint16_t i = 0; i < 256; i++) {
		ops[i].code = i;
		ops[i].opFunc = &CPU::opNotRecognized;
	}

	//ADC Immediate
	setLookup(0x69, "ADC Immediate", &CPU::opADC, AddressMode::Immediate);

	//ADC zero page
	setLookup(0x65, "ADC Zero Page", &CPU::opADC, AddressMode::ZeroPage);

	//ADC zero page x
	setLookup(0x75, "ADC Zero Page,X", &CPU::opADC, AddressMode::ZeroPageX);

	//ADC absolute
	setLookup(0x6D, "ADC Absolute", &CPU::opADC, AddressMode::Absolute);

	//ADC absolute x
	setLookup(0x7D, "ADC Absolute,X", &CPU::opADC, AddressMode::AbsoluteX);

	//ADC absolute y
	setLookup(0x79, "ADC Absolute,Y", &CPU::opADC, AddressMode::AbsoluteY);

	//ADC indirect x
	setLookup(0x61, "ADC (Indirect,X)", &CPU::opADC, AddressMode::IndirectX);

	//ADC indirect y
	setLookup(0x71, "ADC (Indirect),Y", &CPU::opADC, AddressMode::IndirectY);

	//AND immediate
	setLookup(0x29, "AND Immediate", &CPU::opAND, AddressMode::Immediate);

	//AND zero page
	setLookup(0x25, "AND Zero Page", &CPU::opAND, AddressMode::ZeroPage);

	//AND zero page x
	setLookup(0x35, "AND Zero Page,X", &CPU::opAND, AddressMode::ZeroPageX);

	//AND absolute
	setLookup(0x2D, "AND Absolute", &CPU::opAND, AddressMode::Absolute);

	//AND absolute x
	setLookup(0x3D, "AND Absolute,X", &CPU::opAND, AddressMode::AbsoluteX);

	//AND absolute y
	setLookup(0x39, "AND Absolute,Y", &CPU::opAND, AddressMode::AbsoluteY);

	//AND indirect x
	setLookup(0x21, "AND (Indirect,X)", &CPU::opAND, AddressMode::IndirectX);

	//AND indirect y
	setLookup(0x31, "AND (Indirect),Y", &CPU::opAND, AddressMode::IndirectY);

	//ASL accumulator
	setLookup(0x0A, "ASL Accumulator", &CPU::opASL, AddressMode::Accumulator);

	//ASL zero page
	setLookup(0x06, "ASL Zero Page", &CPU::opASL, AddressMode::ZeroPage);

	//ASL zero page x
	setLookup(0x16, "ASL Zero Page,X", &CPU::opASL, AddressMode::ZeroPageX);

	//ASL absolute
	setLookup(0x0E, "ASL Absolute", &CPU::opASL, AddressMode::Absolute);

	//ASL absolute x
	setLookup(0x1E, "ASL Absolute,X", &CPU::opASL, AddressMode::AbsoluteX);

	//BCC relative
	setLookup(0x90, "BCC Relative", &CPU::opBCC, AddressMode::Relative);

	//BCS relative
	setLookup(0xB0, "BCS Relative", &CPU::opBCS, AddressMode::Relative);

	//BEQ relative
	setLookup(0xF0, "BEQ Relative", &CPU::opBEQ, AddressMode::Relative);

	//BIT zero page
	setLookup(0x24, "BIT Zero Page", &CPU::opBIT, AddressMode::ZeroPage);

	//BIT absolute
	setLookup(0x2C, "BIT Absolute", &CPU::opBIT, AddressMode::Absolute);

	//BMI relative
	setLookup(0x30, "BMI Relative", &CPU::opBMI, AddressMode::Relative);

	//BNE relative
	setLookup(0xD0, "BNE Relative", &CPU::opBNE, AddressMode::Relative);

	//BPL relative
	setLookup(0x10, "BPL Relative", &CPU::opBPL, AddressMode::Relative);

	//BRK implied
	setLookup(0x00, "BRK Implied", &CPU::opBRK, AddressMode::Implied);

	//BVC relative
	setLookup(0x50, "BVC Relative", &CPU::opBVC, AddressMode::Relative);

	//BVS relative
	setLookup(0x70, "BVS Relative", &CPU::opBVS, AddressMode::Relative);

	//CLC implied
	setLookup(0x18, "CLC Implied", &CPU::opCLC, AddressMode::Implied);

	//CLD implied
	setLookup(0xD8, "CLD Implied", &CPU::opCLD, AddressMode::Implied);

	//CLI implied
	setLookup(0x58, "CLI Implied", &CPU::opCLI, AddressMode::Implied);

	//CLV implied
	setLookup(0xB8, "CLV Implied", &CPU::opCLV, AddressMode::Implied);

	//CMP immediate
	setLookup(0xC9, "CMP Immediate", &CPU::opCMP, AddressMode::Immediate);

	//CMP zero page
	setLookup(0xC5, "CMP Zero Page", &CPU::opCMP, AddressMode::ZeroPage);

	//CMP zero page x
	setLookup(0xD5, "CMP Zero Page,X", &CPU::opCMP, AddressMode::ZeroPageX);

	//CMP absolute
	setLookup(0xCD, "CMP Absolute", &CPU::opCMP, AddressMode::Absolute);

	//CMP absolute x
	setLookup(0xDD, "CMP Absolute,X", &CPU::opCMP, AddressMode::AbsoluteX);

	//CMP absolute y
	setLookup(0xD9, "CMP Absolute,Y", &CPU::opCMP, AddressMode::AbsoluteY);

	//CMP indirect x
	setLookup(0xC1, "CMP (Indirect,X)", &CPU::opCMP, AddressMode::IndirectX);

	//CMP indirect y
	setLookup(0xD1, "CMP (Indirect),Y", &CPU::opCMP, AddressMode::IndirectY);

	//CPX immediate
	setLookup(0xE0, "CPX Immediate", &CPU::opCPX, AddressMode::Immediate);

	//CPX zero page
	setLookup(0xE4, "CPX Zero Page", &CPU::opCPX, AddressMode::ZeroPage);

	//CPX absolute
	setLookup(0xEC, "CPX Absolute", &CPU::opCPX, AddressMode::Absolute);

	//CPY immediate
	setLookup(0xC0, "CPY Immediate", &CPU::opCPY, AddressMode::Immediate);

	//CPY zero page
	setLookup(0xC4, "CPY Zero Page", &CPU::opCPY, AddressMode::ZeroPage);

	//CPY absolute
	setLookup(0xCC, "CPY Absolute", &CPU::opCPY, AddressMode::Absolute);

	//DEC zero page
	setLookup(0xC6, "DEC Zero Page", &CPU::opDEC, AddressMode::ZeroPage);

	//DEC zero page x
	setLookup(0xD6, "DEC Zero Page,X", &CPU::opDEC, AddressMode::ZeroPageX);

	//DEC absolute
	setLookup(0xCE, "DEC Absolute", &CPU::opDEC, AddressMode::Absolute);

	//DEC absolute x
	setLookup(0xDE, "DEC Absolute,X", &CPU::opDEC, AddressMode::AbsoluteX);

	//DEX implied
	setLookup(0xCA, "DEX Implied", &CPU::opDEX, AddressMode::Implied);

	//DEY implied
	setLookup(0x88, "DEY Implied", &CPU::opDEY, AddressMode::Implied);

	//EOR immediate
	setLookup(0x49, "EOR Immediate", &CPU::opEOR, AddressMode::Immediate);

	//EOR zero page
	setLookup(0x45, "EOR Zero Page", &CPU::opEOR, AddressMode::ZeroPage);

	//EOR zero page x
	setLookup(0x55, "EOR Zero Page,X", &CPU::opEOR, AddressMode::ZeroPageX);

	//EOR absolute
	setLookup(0x4D, "EOR Absolute", &CPU::opEOR, AddressMode::Absolute);

	//EOR absolute x
	setLookup(0x5D, "EOR Absolute,X", &CPU::opEOR, AddressMode::AbsoluteX);

	//EOR absolute y
	setLookup(0x59, "EOR Absolute,Y", &CPU::opEOR, AddressMode::AbsoluteY);

	//EOR Indirect x
	setLookup(0x41, "EOR (Indirect,X)", &CPU::opEOR, AddressMode::IndirectX);

	//EOR indirect y
	setLookup(0x51, "EOR (Indirect),Y", &CPU::opEOR, AddressMode::IndirectY);

	//INC zero page
	setLookup(0xE6, "INC Zero Page", &CPU::opINC, AddressMode::ZeroPage);

	//INC zero page x
	setLookup(0xF6, "INC Zero Page,X", &CPU::opINC, AddressMode::ZeroPageX);

	//INC absolute
	setLookup(0xEE, "INC Absolute", &CPU::opINC, AddressMode::Absolute);

	//INC absolute x
	setLookup(0xFE, "INC Absolute,X", &CPU::opINC, AddressMode::AbsoluteX);

	//INX implied
	setLookup(0xE8, "INX Implied", &CPU::opINX, AddressMode::Implied);

	//INY implied
	setLookup(0xC8, "INY Implied", &CPU::opINY, AddressMode::Implied);

	//JMP absolute
	setLookup(0x4C, "JMP Absolute", &CPU::opJMP, AddressMode::Absolute);

	//JMP indirect
	setLookup(0x6C, "JMP Indirect", &CPU::opJMP, AddressMode::Indirect);

	//JSR absolute
	setLookup(0x20, "JSR Absolute", &CPU::opJSR, AddressMode::Absolute);

	//LDA immediate
	setLookup(0xA9, "LDA Immediate", &CPU::opLDA, AddressMode::Immediate);

	//LDA zero page
	setLookup(0xA5, "LDA Zero Page", &CPU::opLDA, AddressMode::ZeroPage);

	//LDA zero page x
	setLookup(0xB5, "LDA Zero Page,X", &CPU::opLDA, AddressMode::ZeroPageX);

	//LDA absolute
	setLookup(0xAD, "LDA Absolute", &CPU::opLDA, AddressMode::Absolute);

	//LDA absolute x
	setLookup(0xBD, "LDA Absolute,X", &CPU::opLDA, AddressMode::AbsoluteX);

	//LDA absolute y
	setLookup(0xB9, "LDA Absolute,Y", &CPU::opLDA, AddressMode::AbsoluteY);

	//LDA indirect x
	setLookup(0xA1, "LDA (Indirect,X)", &CPU::opLDA, AddressMode::IndirectX);

	//LDA indirect y
	setLookup(0xB1, "LDA (Indirect),Y", &CPU::opLDA, AddressMode::IndirectY);

	//LDX immediate
	setLookup(0xA2, "LDX Immediate", &CPU::opLDX, AddressMode::Immediate);

	//LDX zero page
	setLookup(0xA6, "LDX Zero Page", &CPU::opLDX, AddressMode::ZeroPage);

	//LDX zero page y
	setLookup(0xB6, "LDX Zero Page,Y", &CPU::opLDX, AddressMode::ZeroPageY);

	//LDX absolute
	setLookup(0xAE, "LDX Absolute", &CPU::opLDX, AddressMode::Absolute);

	//LDX absolute y
	setLookup(0xBE, "LDX Absolute,Y", &CPU::opLDX, AddressMode::AbsoluteY);

	//LDY immediate
	setLookup(0xA0, "LDY Immediate", &CPU::opLDY, AddressMode::Immediate);

	//LDY zero page
	setLookup(0xA4, "LDY Zero Page", &CPU::opLDY, AddressMode::ZeroPage);

	//LDY zero page x
	setLookup(0xB4, "LDY Zero Page,X", &CPU::opLDY, AddressMode::ZeroPageX);

	//LDY absolute
	setLookup(0xAC, "LDY Absolute", &CPU::opLDY, AddressMode::Absolute);

	//LDY absolute x
	setLookup(0xBC, "LDY Absolute,X", &CPU::opLDY, AddressMode::AbsoluteX);

	//LSR accumulator
	setLookup(0x4A, "LSR Accumulator", &CPU::opLSR, AddressMode::Accumulator);

	//LSR zero page
	setLookup(0x46, "LSR Zero Page", &CPU::opLSR, AddressMode::ZeroPage);

	//LSR zero page x
	setLookup(0x56, "LSR Zero Page,X", &CPU::opLSR, AddressMode::ZeroPageX);

	//LSR absolute
	setLookup(0x4E, "LSR Absolute", &CPU::opLSR, AddressMode::Absolute);

	//LSR absolute x
	setLookup(0x5E, "LSR Absolute,X", &CPU::opLSR, AddressMode::AbsoluteX);

	//NOP implied
	setLookup(0xEA, "NOP Implied", &CPU::opNOP, AddressMode::Implied);

	//ORA immediate
	setLookup(0x09, "ORA Immediate", &CPU::opORA, AddressMode::Immediate);

	//ORA zero page
	setLookup(0x05, "ORA Zero Page", &CPU::opORA, AddressMode::ZeroPage);

	//ORA zero page x
	setLookup(0x15, "ORA Zero Page,X", &CPU::opORA, AddressMode::ZeroPageX);

	//ORA absolute
	setLookup(0x0D, "ORA Absolute", &CPU::opORA, AddressMode::Absolute);

	//ORA absolute x
	setLookup(0x1D, "ORA Absolute,X", &CPU::opORA, AddressMode::AbsoluteX);

	//ORA absolute y
	setLookup(0x19, "ORA Absolute,Y", &CPU::opORA, AddressMode::AbsoluteY);

	//ORA indirect x
	setLookup(0x01, "ORA (Indirect,X)", &CPU::opORA, AddressMode::IndirectX);

	//ORA indirect y
	setLookup(0x11, "ORA (Indirect),Y", &CPU::opORA, AddressMode::IndirectY);

	//PHA implied
	setLookup(0x48, "PHA Implied", &CPU::opPHA, AddressMode::Implied);

	//PHP implied
	setLookup(0x08, "PHP Implied", &CPU::opPHP, AddressMode::Implied);

	//PLA implied
	setLookup(0x68, "PLA Implied", &CPU::opPLA, AddressMode::Implied);

	//PLP implied
	setLookup(0x28, "PLP Implied", &CPU::opPLP, AddressMode::Implied);

	//ROL accumulator
	setLookup(0x2A, "ROL Accumulator", &CPU::opROL, AddressMode::Accumulator);

	//ROL zero page
	setLookup(0x26, "ROL Zero Page", &CPU::opROL, AddressMode::ZeroPage);

	//ROL zero page x
	setLookup(0x36, "ROL Zero Page,X", &CPU::opROL, AddressMode::ZeroPageX);

	//ROL absolute
	setLookup(0x2E, "ROL Absolute", &CPU::opROL, AddressMode::Absolute);

	//ROL absolute x
	setLookup(0x3E, "ROL Absolute,X", &CPU::opROL, AddressMode::AbsoluteX);

	//ROR accumulator
	setLookup(0x6A, "ROR Accumulator", &CPU::opROR, AddressMode::Accumulator);

	//ROR zero page
	setLookup(0x66, "ROR Zero Page", &CPU::opROR, AddressMode::ZeroPage);

	//ROR zero page x
	setLookup(0x76, "ROR Zero Page,X", &CPU::opROR, AddressMode::ZeroPageX);

	//ROR absolute
	setLookup(0x6E, "ROR Absolute", &CPU::opROR, AddressMode::Absolute);

	//ROR absolute x
	setLookup(0x7E, "ROR Absolute,X", &CPU::opROR, AddressMode::AbsoluteX);

	//RTI implied
	setLookup(0x40, "RTI Implied", &CPU::opRTI, AddressMode::Implied);

	//RTS implied
	setLookup(0x60, "RTS Implied", &CPU::opRTS, AddressMode::Implied);

	//SBC immediate
	setLookup(0xE9, "SBC Immediate", &CPU::opSBC, AddressMode::Immediate);

	//SBC zero page
	setLookup(0xE5, "SBC Zero Page", &CPU::opSBC, AddressMode::ZeroPage);

	//SBC zero page x
	setLookup(0xF5, "SBC Zero Page,X", &CPU::CPU::opSBC, AddressMode::ZeroPageX);

	//SBC absolute
	setLookup(0xED, "SBC Absolute", &CPU::opSBC, AddressMode::Absolute);

	//SBC absolute x
	setLookup(0xFD, "SBC Absolute,X", &CPU::opSBC, AddressMode::AbsoluteX);

	//SBC absolute y
	setLookup(0xF9, "SBC Absolute,Y", &CPU::opSBC, AddressMode::AbsoluteY);

	//SBC indirect x
	setLookup(0xE1, "SBC (Indirect,X)", &CPU::opSBC, AddressMode::IndirectX);

	//SBC indirect y
	setLookup(0xF1, "SBC (Indirect),Y", &CPU::opSBC, AddressMode::IndirectY);

	//SEC implied
	setLookup(0x38, "SEC Implied", &CPU::opSEC, AddressMode::Implied);

	//SED implied
	setLookup(0xF8, "SED Implied", &CPU::opSED, AddressMode::Implied);

	//SEI implied
	setLookup(0x78, "SEI Implied", &CPU::opSEI, AddressMode::Implied);

	//STA zero page
	setLookup(0x85, "STA Zero page", &CPU::opSTA, AddressMode::ZeroPage);

	//STA zero page x
	setLookup(0x95, "STA Zero Page,X", &CPU::opSTA, AddressMode::ZeroPageX);

	//STA absolute
	setLookup(0x8D, "STA Absolute", &CPU::opSTA, AddressMode::Absolute);

	//STA absolute x
	setLookup(0x9D, "STA Absolute,X", &CPU::opSTA, AddressMode::AbsoluteX);

	//STA absolute y
	setLookup(0x99, "STA Absolute,Y", &CPU::opSTA, AddressMode::AbsoluteY);

	//STA indirect x
	setLookup(0x81, "STA (Indirect,X)", &CPU::opSTA, AddressMode::IndirectX);

	//STA indirect y
	setLookup(0x91, "STA (Indirect),Y", &CPU::opSTA, AddressMode::IndirectY);

	//STX zero page
	setLookup(0x86, "STX Zero Page", &CPU::opSTX, AddressMode::ZeroPage);

	//STX zero page y
	setLookup(0x96, "STX Zero Page,Y", &CPU::opSTX, AddressMode::ZeroPageY);

	//STX absolute
	setLookup(0x8E, "STX Absolute", &CPU::opSTX, AddressMode::Absolute);

	//STY zero page
	setLookup(0x84, "STX Zero Page", &CPU::opSTY, AddressMode::ZeroPage);

	//STY zero page x
	setLookup(0x94, "STY Zero Page,X", &CPU::opSTY, AddressMode::ZeroPageX);

	//STY absolute
	setLookup(0x8C, "STY Absolute", &CPU::opSTY, AddressMode::Absolute);

	//TAX implied
	setLookup(0xAA, "TAX Implied", &CPU::opTAX, AddressMode::Implied);

	//TAY implied
	setLookup(0xA8, "TAY Implied", &CPU::opTAY, AddressMode::Implied);

	//TSX implied
	setLookup(0xBA, "TSX Implied", &CPU::opTSX, AddressMode::Implied);

	//TXA implied
	setLookup(0x8A, "TXA Implied", &CPU::opTXA, AddressMode::Implied);

	//TXS implied
	setLookup(0x9A, "TXS Implied", &CPU::opTXS, AddressMode::Implied);

	//TYA implied
	setLookup(0x98, "TYA Implied", &CPU::opTYA, AddressMode::Implied);
}

//Appropriate interrupt vector stored in addressTemp
void CPU::performInterrupt() {
	if (cycleCounter == 0) {
		//Dummy read
		console->cpuRead(programCounter);
		cycleCounter++;
	}
	else if (cycleCounter == 1) {
		//Push PCH to stack
		console->cpuWrite(0x0100 + stackPointer, (programCounter & 0xFF00) >> 8);
		stackPointer--;
		cycleCounter++;
	}
	else if (cycleCounter == 2) {
		//Push PCL to stack
		console->cpuWrite(0x0100 + stackPointer, programCounter & 0x00FF);
		stackPointer--;
		cycleCounter++;
	}
	else if (cycleCounter == 3) {
		//Push P to stack
		console->cpuWrite(0x0100 + stackPointer, (status | 0x20) & 0xEF);
		stackPointer--;
		setInterruptFlag();
		cycleCounter++;
	}
	else if (cycleCounter == 4) {
		//fetch ADL
		programCounter = 0x0000 | console->cpuRead(addressTemp);
		cycleCounter++;
	}
	else if (cycleCounter == 5) {
		//fetch ADH
		programCounter |= console->cpuRead(addressTemp+1) << 8;
		cycleCounter = 0;
		inInterrupt = false;
	}
}

bool CPU::interruptWaiting() {
	return irqWaiting | nmiWaiting | resetWaiting;
}

CPU::CPU(Console *con) {
	initializeLookups();

	console = con;
}

void CPU::cycle() {
	if (inInstruction) {
		//perform the function for the current op
		(this->*(ops[currentOp].opFunc))();
	}
	else if (inInterrupt) {
		performInterrupt();
	}
	else if (interruptWaiting()) {
		inInterrupt = true;
		if (resetWaiting) {
			addressTemp = 0xFFFC;
			resetWaiting = false;
		}
		else if (nmiWaiting) {
			addressTemp = 0xFFFA;
			nmiWaiting = false;
		}
		else if (irqWaiting) {
			addressTemp = 0xFFFE;
			irqWaiting = false;
		}
	}
	else {
		inInstruction = true;
		currentOp = console->cpuRead(programCounter);
		currentMode = ops[currentOp].mode;
		programCounter++;
	}
}

void CPU::raiseIRQ() {

	irqWaiting = true;
}
void CPU::raiseNMI() {
	nmiWaiting = true;
}
void CPU::raiseReset() {
	resetWaiting = true;
}

//Debug functions
uint8_t CPU::getX() {
	return x;
}
void CPU::setX(uint8_t x) {
	this->x = x;
}

uint8_t CPU::getY() {
	return y;
}
void CPU::setY(uint8_t y) {
	this->y = y;
}

uint8_t CPU::getAcc() {
	return acc;
}
void CPU::setAcc(uint8_t acc) {
	this->acc = acc;
}

uint8_t CPU::getStatus() {
	return status;
}
void CPU::setStatus(uint8_t status) {
	this->status = status;
}

uint8_t CPU::getStackPointer() {
	return stackPointer;
}
void CPU::setStackPointer(uint8_t stackPointer) {
	this->stackPointer = stackPointer;
}

uint16_t CPU::getProgramCounter() {
	return programCounter;
}
void CPU::setProgramCounter(uint16_t programCounter) {
	this->programCounter = programCounter;
}

Operation CPU::performNextInstruction() {
	Operation ret;
	if (!inInstruction) {
		cycle();
		ret = ops[currentOp];
	}
	while (inInstruction)
		cycle();
	return ret;
}

uint8_t CPU::readWord(uint16_t address) {
	return console->debugRead(address);
}