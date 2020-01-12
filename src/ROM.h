#ifndef ROM_H
#define ROM_H

#include <string>
#include <cstdint>
#include <exception>

#define TRAINER_SIZE 512

using namespace std;

class Mapper;
class Console;

class RomLoadingException : public std::exception {
private:
	string msg;

public:
	RomLoadingException(string msg) {
		this->msg = msg;
	}

	string getMessage() {
		return msg;
	}
};

class RomImage {
private:
	uint8_t flags6;

	uint8_t flags7;

	uint16_t mapper;

	uint16_t prgSize;

	uint16_t chrSize;

	uint32_t prgRamSize;

	uint32_t prgNVRamSize;

	uint32_t chrRamSize;

	uint32_t chrNVRamSize;

	uint8_t miscRomNum;

	uint64_t miscRomSize;

	uint8_t *trainer;

	uint8_t *prgROM;

	uint8_t *chrROM;

	uint8_t *miscROM;

public:
	//returns the pointer to PRG-ROM
	uint8_t *getPrgRom();

	//Returns true if the PRG-ROM size is in
	//exponential format
	bool prgExpFormat();

	uint64_t getPrgRomSize();

	//Returns the pointer to CHR-ROM
	uint8_t *getChrRom();

	//Returns true if the CHR-ROM size is in
	//exponential format
	bool chrExpFormat();

	uint64_t getChrRomSize();

	//returns the pointer to the miscellaneous ROM section
	uint8_t *getMiscRom();

	uint64_t getMiscSize();

	//Returns mapper, with submapper in high nybble
	uint16_t getMapperNumber();

	bool trainerPresent();

	//True if ROM is in NES 2.0 format
	bool isNES2();

	//Returns true if nametable mirroring bit
	//is 0, which can also mean that mirroring
	//is mapper controlled
	bool horizontalMirroring();

	//Returns true if nametable mirroring bit
	//is 1
	bool verticalMirroring();

	//True if the hardwaired four screen mirroring bit is set
	bool fourScreenMirroring();
	
	//True if 'battery present' bit is set
	bool batteryPresent();

	uint32_t getPrgRamSize();

	uint32_t getPrgNVRamSize();

	uint32_t getChrRamSize();

	uint32_t getChrNVRamSize();

	//path is the path to the rom file
	//
	//if forceload is false, attempting to load an image
	//	which uses unsupported features results in an exception
	//if forceload is true, the image will be loaded regardless
	//	of what features it uses
	RomImage(string path, bool forceload);

	~RomImage();

	//Returns the mapper object specified by the ROM
	//attempting to load a ROM which uses an unsupported
	//mapper results in an exception
	Mapper *getMapper(Console *con);
};

#endif