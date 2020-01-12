#include "ROM.h"
#include "Mapper.h"
#include "MapperHeaders.h"

#include <fstream>
#include <cstring>
#include <cstdlib>

#define INES_FILE_ID "NES\x1A"

uint8_t *RomImage::getPrgRom() {
	return prgROM;
}

//Returns true if the PRG-ROM size is in
//exponential format
bool RomImage::prgExpFormat() {
	return (prgSize & 0x0F00) == 0x0F00;
}

//if prgExpFormat is true, return value is in bytes
//else it is in 16KB units
uint64_t RomImage::getPrgRomSize() {
	if (prgExpFormat()) {
		uint64_t exp = (prgSize & 0x00FC) >> 2;
		uint64_t mul = prgSize & 0x0003;
		return (2*mul + 1) * (1 << exp);
	}
	else {
		return prgSize;
	}
}

uint8_t *RomImage::getChrRom() {
	return chrROM;
}

//Returns true if the CHR-ROM size is in
//exponential format
bool RomImage::chrExpFormat() {
	return (chrSize & 0x0F00) == 0x0F00;
}

//if chrExpFormat is true, return value is in bytes
//else it is in 16KB units
uint64_t RomImage::getChrRomSize() {
	if (prgExpFormat()) {
		uint64_t exp = (chrSize & 0x00FC) >> 2;
		uint64_t mul = chrSize & 0x0003;
		return (2*mul + 1) * (1 << exp);
	}
	else {
		return prgSize;
	}
}

//returns the pointer to the miscellaneous ROM section
uint8_t *RomImage::getMiscRom() {
	return miscROM;
}

uint64_t RomImage::getMiscSize() {
	return miscRomSize;
}

//Returns mapper, with submapper in high nybble
uint16_t RomImage::getMapperNumber() {
	return mapper;
}

bool RomImage::trainerPresent() {
	return flags6 & 0x04;
}

//True if ROM is in NES 2.0 format
bool RomImage::isNES2() {
	return (flags7 & 0x0C) == 0x080;
}

//Returns true if nametable mirroring bit
//is 0, which can also mean that mirroring
//is mapper controlled
bool RomImage::horizontalMirroring() {
	return !(flags6 & 0x01);
}

//Returns true if nametable mirroring bit
//is 1
bool RomImage::verticalMirroring() {
	return flags6 & 0x01;
}

//True if the hardwaired four screen mirroring bit is set
bool RomImage::fourScreenMirroring() {
	return flags6 & 0x08;
}

//True if 'battery present' bit is set
bool RomImage::batteryPresent() {
	return flags6 & 0x02;
}

uint32_t RomImage::getPrgRamSize() {
	return prgRamSize;
}

uint32_t RomImage::getPrgNVRamSize() {
	return prgNVRamSize;
}

uint32_t RomImage::getChrRamSize() {
	return chrRamSize;
}

uint32_t RomImage::getChrNVRamSize() {
	return chrNVRamSize;
}

//path is the path to the rom file
//
//if forceload is false, attempting to load an image
//	which uses unsupported features results in an exception
//if forceload is true, the image will be loaded regardless
//	of what features it uses
RomImage::RomImage(string path, bool forceload) {
	//Open rom file
	ifstream romFile(path, ios::in | ios::binary);

	if (!romFile.is_open()) {
		throw(RomLoadingException("Failed to open ROM file"));
	}

	//Check to make sure the file is in iNES or NES 2.0 format
	char fileID[5];
	fileID[4] = 0;
	romFile.read(fileID, 4);
	//if file does not begin with "NES\x1A", throw exception
	if (strcmp(fileID, INES_FILE_ID) != 0) {
		throw(RomLoadingException("Tried to load ROM not in iNES or NES 2.0 format"));
	}

	uint8_t buff;

	//Set least significant byte for PRG-ROM size
	romFile >> buff;
	prgSize = 0x0000 | buff;

	//Set least significant byte for CHR-ROM size
	romFile >> buff;
	chrSize = 0x0000 | buff;

	//Load flags6 and set low nybble on mapper
	romFile >> flags6;
	mapper = 0x0000 | (flags6 >> 4);

	//load flags7 and set middle nybble on mapper
	romFile >> flags7;
	mapper |= flags7 & 0xF0;
	//check if console type is 0 (NES/Famicom)
	if ((flags7 & 0x03) != 0) {
		if (!forceload) {
			throw(RomLoadingException("ROM console type not supported"));
		}
	}

	//NES 2.0 specific header information
	if (isNES2()) {
		//load mapper high nybble and submapper
		romFile >> buff;
		mapper |= buff << 8;

		//get high nybble for prg and chr ROM
		romFile >> buff;
		prgSize |= (buff & 0x0F) << 8;
		chrSize |= (buff & 0xF0) << 4;

		//load PRG-(NV)RAM shift counters and calculate size
		romFile >> buff;
		prgRamSize = 64 << (buff & 0x0F);
		prgNVRamSize = 64 << ((buff & 0xF0) >> 4);

		//load CHR-(NV)RAM shift counters and calculate size
		romFile >> buff;
		chrRamSize = 64 << (buff & 0x0F);
		chrNVRamSize = 64 << ((buff & 0xF0) >> 4);

		//check for CPU/PPU timing
		romFile >> buff;
		buff &= 0x03;
		if (buff != 0x00 && buff != 0x02) {
			throw(RomLoadingException("Unsupported CPU/PPU timing"));
		}

		//Vs system/extended console type not supported
		romFile >> buff;

		//miscellaneous rom number
		romFile >> miscRomNum;

		//check expansion device
		romFile >> buff;
		buff &= 0x3F;
		if (buff != 0x00 && buff != 0x01) {
			throw(RomLoadingException("Unsupported default expansion device"));
		}
	}
	else {
		//iNES assumes if chrROM size is 0, then the cartridge has 8KB of chrRAM
		//while NES 2.0 requires that all chrRAM is specified is byte 11
		if (chrSize == 0) {
			chrRamSize = 0x2000;
		}
		prgRamSize = 0x2000;
		romFile.seekg(8, ios_base::cur);
	}

	//if trainer is present load into memory
	if (trainerPresent()) {
		trainer = (uint8_t *) malloc(TRAINER_SIZE);
		romFile.read((char *)trainer, TRAINER_SIZE);
	}
	else {
		trainer = NULL;
	}

	//calculate size of PRG-ROM in bytes
	uint64_t prgByteSize = getPrgRomSize();
	if (!prgExpFormat()) {
		//when size is not in exponential format
		//the size is specified in 16KB units
		prgByteSize *= 0x4000;
	}

	if (prgByteSize != 0) {
		prgROM = (uint8_t *)malloc(prgByteSize);
		romFile.read((char *)prgROM, prgByteSize);
	}
	else {
		prgROM = NULL;
	}

	//calculate size of CHR-ROM in bytes
	uint64_t chrByteSize = getChrRomSize();
	if (!chrExpFormat()) {
		//when size is not in exponential format
		//the size is specified in 16KB units
		chrByteSize *= 0x4000;
	}

	if (prgByteSize != 0) {
		chrROM = (uint8_t *) malloc(chrByteSize);
		romFile.read((char *)chrROM, chrByteSize);
	}
	else {
		chrROM = NULL;
	}

	//Rest of the file goes to miscROM
	//Calculate distance to end of file
	streampos current = romFile.tellg();
	romFile.seekg(0, ios_base::end);
	streampos end = romFile.tellg();
	romFile.seekg(current);
	uint64_t miscSize = end-current;

	if (miscSize != 0) {
		miscROM = (uint8_t *) malloc(miscSize);
		romFile.read((char *)miscROM, miscSize);
	}
	else {
		miscROM = NULL;
	}

	romFile.close();
}

RomImage::~RomImage() {
	delete trainer;
	delete prgROM;
	delete chrROM;
	delete miscROM;
}

//Returns the mapper object specified by the ROM
//attempting to load a ROM which uses an unsupported
//mapper results in an exception
Mapper *RomImage::getMapper(Console *con) {
	Mapper *ret;
	string msg;
	switch (mapper & 0x0FFF) {
		case 0:
			ret = new NROM(con, this);
			break;
		default:
			throw(RomLoadingException("Unsupported mapper number"));
			break;
	}
	return ret;
}