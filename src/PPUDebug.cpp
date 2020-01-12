//This class is purely for testing the PPU code
//there's probablyy a better way to do this, but this is quick

#include "PPUDebug.h"
//#include "Console.h"
#include <cstring>
#include <cstdlib>
#include <iostream>

using namespace std;

void PPU::writeVRAM(uint16_t address, uint8_t data) {
	vram[address & 0x3FFF] = data;
}

//Maintainability placeholder until I develop more sophisticated memory handling
uint8_t PPU::readVRAM(uint16_t address) {
	return vram[address & 0x3FFF];
}

//Retrieves appropriate byte in name table based on rendering address
//Bit manipulation based on how the accessAddress register is used
//during rendering. See description of rendering (wherever I end up putting that)
uint8_t PPU::retrieveNameTableByte(uint16_t address) {
	return readVRAM((address & 0x0FFF) | 0x2000);	
}

//Attribute table contains the upper two bits of the palette entry
//It is 64 bytes in size, creating an 8*8 grid which divides the screen
//Into 4*4 tile groups corresponding to each byte. Each of those groups is
//further divided into 4 2*2 squares. Each of these squares corresponds to 
//two bits in the byte. The bits are layed out like so: 33221100 corresponding
//to the four squares which are layed out like so:
//			---------------------------------
//			|	Square 0	|	Square 1	|
//			|	$0	$1		|	$4	$5		|
//			|	$2	$3		|	$6	$7		|
//			---------------------------------
//			|	Square 2	|	Square 3	|
//			|	$8	$9		|	$C 	$D 		|
//			|	$A 	$B 		|	$E 	$F 		|
//			---------------------------------
//All of which is horrendously complicated
uint8_t PPU::retrieveAttrTableBits(uint16_t address) {
	//Calculates the address of the appropriate byte in the attribute table
	//						   Nametable region offset   Attr offset    Nametable select     Y select                    X Select									
	uint16_t attributeAddress = 0x2000                  | 0x03C0       | (address & 0x0C00) | ((address & 0x0380) >> 4) | ((address & 0x001C) >> 2);
	//Calculates which square within the byte that the tile belongs to
	//					   X coordinate				   Y coordinate
	uint8_t squareNumber = ((address & 0x0002) >> 1) | ((address & 0x0040) >> 5);
	
	//DEBUG
	//cout << "Attr fetch:" << endl << "\t" << hex << attributeAddress << ": " << +readVRAM(attributeAddress) << endl;
	
	//Retrieves the byte from VRAM, shifts the appropriate bits to the end and masks the rest away
	return (readVRAM(attributeAddress) >> (2 * squareNumber)) & 0x03;
}

//Pattern table addresses are structured like so:
//	0HBBBBBBBBPTTT
//	|||||||||||+++-- T: Fine Y offset, the row number within a tile
//	||||||||||+----- P: Bit plane (0: "lower"; 1: "upper")
//	||++++++++------ B:	Pattern identifer byte
//	|+-------------- H: Half of pattern table (0: "left"; 1: "right")
//	+--------------- 0: Pattern table is at $0000-$1FFF
//The arguments split that up in a fairly strightforward manner
//and it returns the the bits corresponding to the appropriate part of the
//appropriate pattern. See rendering function for more detailed description of
//what these bits mean and how they are used
uint8_t PPU::retrievePatternTableByte(bool patternTable, uint8_t patternByte, bool plane, uint8_t yOffset) {
	uint16_t patternAddress = (patternTable << 12) | (patternByte << 4) | (plane << 3) | yOffset;
	
	//DEBUG
	cout << "PT fetch:" << endl << "\t" << hex << patternAddress << ": " << +readVRAM(patternAddress) << endl;

	return readVRAM(patternAddress);
}

//The following several functions were pulled out of the cycle function for readability
//Halfway through inplementing cycle it become an unmanagable tangle of nested ifs

void PPU::incrementHorizontal() {
	//get current coarse x
	uint8_t x = accessAddress & 0x001F;
	//increment it
	x++;
	//did x overflow?
	if (x == 0x20)
	{
		//reset x to 0
		x = 0;
		//flip horizontal nametable bit in address
		accessAddress ^= 0x0400;
	}
	//clear coarse x
	accessAddress &= 0xFFE0;
	//set new coarse x
	accessAddress |= x; 
}

void PPU::incrementVertical() {
	//Check to see if fine y will overflow
	if ((accessAddress & 0x7000) != 0x7000)
		//if it doesn't, just increment fine y
		accessAddress += 0x1000;
	else {
		//else set fine y to 0
		accessAddress &= 0x0FFF;
		//grab coarse y
		uint8_t y = (accessAddress & 0x03E0) >> 5;

		//Coarse Y wraparound behavior is slightly complicated
		//if y is 29, it wraps around to 0 and the vertical nametable is switched
		//During normal rendering y should never be more than 29
		//However, it can be set manually to 30 or 31
		//If this is encountered during y increment, 30 is incremented to 31
		//and 31 is wrapped around to 0 WITHOUT switching vertical nametables
		if (y == 29) {
			y = 0;
			accessAddress ^= 0x8000;
		}
		else if (y == 31) {
			y = 0;
		}
		else {
			y++;
		}

		//Finally, clear the old coarse y and set it to the new one
		accessAddress &= 0x7C1F;
		accessAddress |= y << 5;
	}
}

uint8_t reverseByte(uint8_t n) {
	uint8_t ret = 0;
	uint8_t mask = 0x01;
	for (int i = 0; i < 8; i++, mask <<= 1) {
		ret <<= 1;
		ret |= ((n & mask) != 0);
	}
	return ret;
}

void PPU::fetchBGTile() {
	//cout << +cycles << "\t" << +cycles % 8 << endl;
	//Nametable fetch
	if (cycles % 8 == 2) {
		currentPattern = readVRAM((accessAddress & 0x0FFF) | 0x2000);
		
		//DEBUG
		//cout << "Tile #" << +cycles/8 << endl;
		//cout << "NT fetch:" << endl << "\t" << hex << ((accessAddress & 0x0FFF) | 0x2000) << ": " << +currentPattern << endl;
	}
	//Attribute fetch
	else if (cycles % 8 == 4) {
		//cout << "DEBUG" << endl;
		attrLatchBuffer = retrieveAttrTableBits(accessAddress);
	}
	//Lower pattern table fetch
	else if (cycles % 8 == 6) {
		patternBuffer0 = reverseByte(retrievePatternTableByte(ppuControl1 & CONTROL1_BG_PT, currentPattern, 0, accessAddress >> 12));
	}
	//Higher pattern table fetch/course X increment
	else if (cycles % 8 == 0) {
		patternBuffer1 = reverseByte(retrievePatternTableByte(ppuControl1 & CONTROL1_BG_PT, currentPattern, 1, accessAddress >> 12));
		
		incrementHorizontal();
	}
}

void PPU::fetchSpriteData() {
	//calculates which sprite object from secondary oam
	//to pull data from
	int16_t currentSprite = (cycles - 257)/8;
	//Lower pattern table fetch
	if (cycles % 8 == 6) {
		//Sprite index was used during sprite evaluation to keep track
		//of how many sprites have been found for the current scanline
		//So if we have more sprites to load, grab from secondary OAM
		//Otherwise fill with transparent data
		if (currentSprite < spriteIndex) {
			//If sprites are 8x16 the pattern table select bit is ignored
			//and bit 0 from the tile index is used to indicate the appropriate pattern table
			//bit 4 from the range comparison is then used to set bit 0 of the tile index
			if (ppuControl1 & CONTROL1_SPR_SIZE) {
				spriteShift[currentSprite*2+0] = reverseByte(retrievePatternTableByte(oamSecondary[currentSprite*4+1] & 0x01, (oamSecondary[currentSprite*4+1] & 0xFE) | ((oamSecondary[currentSprite*4+0] & 0x08) >> 3), 0, oamSecondary[currentSprite*4+0] & 0x07));
			}
			else {
				spriteShift[currentSprite*2+0] = reverseByte(retrievePatternTableByte(ppuControl1 & CONTROL1_SPR_PT, oamSecondary[currentSprite*4+1], 0, oamSecondary[currentSprite*4+0] & 0x07));
			}
		}
		else {
			spriteShift[currentSprite*2+0] = 0x00;
		}
	}
	//Higher pattern table fetch
	else if (cycles % 8 == 0) {
		if (currentSprite < spriteIndex) {
			if (ppuControl1 & CONTROL1_SPR_SIZE) {
				spriteShift[currentSprite*2+1] = reverseByte(retrievePatternTableByte(oamSecondary[currentSprite*4+1] & 0x01, (oamSecondary[currentSprite*4+1] & 0xFE) | ((oamSecondary[currentSprite*4+0] & 0x08) >> 3), 1, oamSecondary[currentSprite*4+0] & 0x07));
			}
			else {
				spriteShift[currentSprite*2+1] = reverseByte(retrievePatternTableByte(ppuControl1 & CONTROL1_SPR_PT, oamSecondary[currentSprite*4+1], 1, oamSecondary[currentSprite*4+0] & 0x07));
			}
			spriteAttr[currentSprite] = oamSecondary[currentSprite*4+2];
			spriteXCounter[currentSprite] = oamSecondary[currentSprite*4+3];
		}
		else {
			spriteShift[currentSprite*2+1] = 0x00;
			spriteAttr[currentSprite] = 0x20; //behind background
			spriteXCounter[currentSprite] = 0xFF; //At right edge of screen
		}
	}
}

void PPU::loadSprites() {
	if (!readingSprite) {
		//Find position of the top of the sprite relative to the current scanline
		int16_t range = scanline - oam[spriteMemAddress];
		//cout << "Checking sprite at " << hex << spriteMemAddress << endl;
		//cout << "\t" << dec << range << endl;
		
		//If we have 8x16 sprites, we check 0 <= range <= 15
		if (ppuControl1 & CONTROL1_SPR_SIZE) {
			if (range >= 0 && range <= 15) {
				//oamSecondary[spriteIndex*4+0] = oam[spriteMemAddress];
				oamSecondary[spriteIndex*4+0] = range;
				//If the sprite found is sprite 0, set the tracker
				if (spriteMemAddress == 0) {
					//Sprite 0 hit is not detected at x = 255 or at 0-7 if the background
					//or sprites are hidden in this area
					if (! (oam[spriteMemAddress+3] == 255 || (ppuControl2 & CONTROL2_CLIP_LEFT && oam[spriteMemAddress+3] >= 0 && oam[spriteMemAddress+3] < 8)))
						sprite0Tracker |= 0x02;
				}
				spriteMemAddress +=1;
				readingSprite = 3;
			}
			else {
				spriteMemAddress += 4;
			}
		}
		//If we have 8x8 sprites, we check 0 <= range <= 7
		else {
			if (range >= 0 && range <= 7) {
				//cout << "\tFound" << endl;
				//oamSecondary[spriteIndex*4+0] = oam[spriteMemAddress];
				oamSecondary[spriteIndex*4+0] = range;
				//If the sprite found is sprite 0, set the tracker
				if (spriteMemAddress == 0) {
					//Sprite 0 hit is not detected at x = 255 or at 0-7 if the background
					//or sprites are hidden in this area
					if (! (oam[spriteMemAddress+3] == 255 || (ppuControl2 & CONTROL2_CLIP_LEFT && oam[spriteMemAddress+3] >= 0 && oam[spriteMemAddress+3] < 8)))
						sprite0Tracker |= 0x02;
				}
				spriteMemAddress += 1;
				readingSprite = 3;
			}
			else {
				//cout << "\tMissed" << endl;
				spriteMemAddress += 4;
			}
		}
	}
	//Copy tile index byte
	else if (readingSprite == 3) {
		oamSecondary[spriteIndex*4+1] = oam[spriteMemAddress];
		spriteMemAddress +=1;
		readingSprite -= 1;
	}
	//Copy attribute byte
	else if (readingSprite == 2) {
		oamSecondary[spriteIndex*4+2] = oam[spriteMemAddress];
		//if the vertical flip byte is set, bitwise invert the range
		if (oamSecondary[spriteIndex*4+2] & 0x80) {
			oamSecondary[spriteIndex*4+0] = (~oamSecondary[spriteIndex*4+0]) & 0x0F;
		}
		spriteMemAddress +=1;
		readingSprite -= 1;
	}
	else if (readingSprite == 1) {
		oamSecondary[spriteIndex*4+3] = oam[spriteMemAddress];
		//DEBUG
		cout << "Sprite " << +spriteIndex << " found" << endl;
		cout << "\tY: " << hex << +(oamSecondary[spriteIndex*4+0]) << endl;
		cout << "\tTile index: " << +oamSecondary[spriteIndex*4+1] << endl;
		cout << "\tAttr: " << +oamSecondary[spriteIndex*4+2] << endl;
		cout << "\tX: " << +oamSecondary[spriteIndex*4+3] << endl;
		spriteMemAddress += 1;
		spriteIndex += 1;
		readingSprite -= 1;
	}
}

void PPU::checkSpriteOverflow() {
	//If sprite overflow evaluation detects an in-range sprite, it
	//reads the next 3 bytes (for some reason) instead of just jumping
	//to the next sprite. As far as I can tell these are just some sort
	//of garbage reads that don't affect anything. Probably something to
	//do with reusing the hardware for normal sprite evaluation.
	//Regardless, because of the address increment bug, the logic
	//I used for normal sprite evaluation no longer works here
	//So 'readingSprite' is just a counter that gets set to 3 when
	//an in-range sprite is detected and decremented until it reaches 0
	//Simulating the 3 garbage r/w cycles
	if (!readingSprite) {
		//Find position of the top of the sprite relative to the current scanline
		int16_t range = scanline - oam[spriteMemAddress];
		//If we have 8x16 sprites, we check 0 <= range <= 15
		if (ppuControl1 & CONTROL1_SPR_SIZE) {
			if (range >= 0 && range <= 15) {
				ppuStatus |= STATUS_SPR_OVFLW;
				spriteMemAddress += 4;
				readingSprite = 3;
			}
			else {
				//So here the particulars of the internal representation of the OAM 
				//address become relevant, and this is why the original logic fails
				//The 8-byte sprite memory address can be thought of as divided into
				//two sub-registers, n (bits 7-2) and m (bits 0-1). Normally they act
				//as one 8-byte register, but when checking for sprite overflow, if
				//the value checked is not in range, both both n and m are incremented
				//without carry on m. So when m does not overflow, 0x101 = 5 is added.
				//But when m = 3, it wraps around to 0, so effectively 3 is subtracted
				//from the address. After the +4 from the n increment this becomes a
				//a net +1 to the address. This m increment causes bytes that aren't y 
				//coordinates to be evaluated as if they were, making the sprite overflow
				//flag unreliable
				if (spriteMemAddress % 4 == 3)
					spriteMemAddress += 1;
				else
					spriteMemAddress += 5;
			}
		}
		//If we have 8x8 sprites, we check 0 <= range <= 7
		else {
			if (range >= 0 && range <= 7) {
				ppuStatus |= STATUS_SPR_OVFLW;
				spriteMemAddress += 4;
				readingSprite = 3;
			}
			else {
				if (spriteMemAddress % 4 == 3)
					spriteMemAddress += 1;
				else
					spriteMemAddress += 5;
			}
		}
	}
	else {
		readingSprite -= 1;
	}
}

bool PPU::isTransparent(uint8_t pixel) {
	return (pixel & 0x03) == 0x00;
}

void PPU::calculatePixel() {
	uint16_t xSelector = 0x01 << fineX;

	//Calculate the background pixel palette value
	uint8_t bgPixel = 0x00 | (((attrShift1 & xSelector) != 0) << 3) | (((attrShift0 & xSelector) != 0) << 2) | (((patternShift1 & xSelector) != 0) << 1) | (((patternShift0 & xSelector) != 0) << 0);

	//DEBUG
	//cout << "Pixel #" << dec << +cycles << endl;
	//cout << "\t" << hex << +readVRAM(bgPixel | 0x3F00) << endl;

	uint8_t sprPixel = 0x00;

	//holds sprite priority bit
	bool priority = 0;

	//Loop through sprites to see if there is a sprite
	for (int i = 0; i < 8; i++) {
		//If we've reached the x coordinate of the sprite, check it and shift the shift registers
		if (spriteXCounter[i] == 0) {
			if (isTransparent(sprPixel)) {
				priority = spriteAttr[i] & 0x10;
				sprPixel = 0x00 | ((spriteAttr[i] & 0x03) << 2) | ((spriteShift[2*i+1] & 0x01) << 1) | ((spriteShift[2*i+0] & 0x01) << 0);
			}
			//determine whether to raise sprite0 hit flag
			if (i == 0 && ((sprite0Tracker & 0x01) == 0x01) && !isTransparent(sprPixel) && !isTransparent(bgPixel)) {
				ppuStatus |= STATUS_SPR0_HIT;
			} 
			spriteShift[2*i+0] >>= 1;
			spriteShift[2*i+1] >>= 1;
		}
		//Otherwise decrement the x counter
		else {
			spriteXCounter[i] -= 1;
		}
	}

	//determine whether to use sprite pixel or background pixel
	if ((ppuControl2 & CONTROL2_BG_RNDR) && (!(ppuControl2 & CONTROL2_SPR_RNDR))) {
		pixelBuffer[cycles-1] = readVRAM(bgPixel | 0x3F00);
	}
	else if ((!(ppuControl2 & CONTROL2_BG_RNDR)) && (ppuControl2 & CONTROL2_SPR_RNDR)) {
		pixelBuffer[cycles-1] = readVRAM(sprPixel | 0x3F10);
	}
	else {
		if ((priority == 0 || isTransparent(bgPixel)) && !isTransparent(sprPixel))
			pixelBuffer[cycles-1] = readVRAM(sprPixel | 0x3F10);
		else
			pixelBuffer[cycles-1] = readVRAM(bgPixel | 0x3F00);
	}
}

//Increments the cycle and scanline counters
void PPU::incrementCycle() {
	cycles++;
	frameEnd = false;
	//Wrap-around cycles to 0 and increment line after 340
	if (cycles > 340) {
		scanline++;
		cout << "Scanline: " << dec << scanline << endl;
		cycles = 0;
		//Wrap-around scanlines to 0 and flip frame parity after line 261 
		if (scanline > 261) {
			scanline = 0;
			//On odd frames skip cycle 0 on first scanline
			if (frameParity = 0) {
				cycles = 1;
			}
			frameParity = ~frameParity;
			frameEnd = true;
		}
	}

	//decrement countdown
	if (resetCountdown > 0)
		resetCountdown--;
}

//This function performs a PPU cycle
//3 things are happening (more or less) in parallel:
	//1. Memory fetches
	//2. Sprite evalutation
	//3. Pixel muxing
//Accordingly there are 3 corresponding sections of this function
//The following is a diagram of how the access address  is used during rendering:
//yyy NN YYYYY XXXXX
//||| || ||||| +++++-- coarse X scroll
//||| || +++++-------- coarse Y scroll
//||| ++-------------- nametable select
//+++----------------- fine Y scroll
//
//NOTE: Because the PPU has to operate cycle by cycle, this function rapidly became
//a rat's nest of if statements determining what state the PPU is in. Hopefully I'll come
//back to this some time in the future and fix it up to be more readable and maintainable
void PPU::cycle() {
	//Code for non-rendering/pre-rendering period
	if (scanline > 239) {
		if (scanline == 241 && cycles == 1) {
			ppuStatus |= STATUS_VBL;
			if (ppuControl1 & CONTROL1_VBL) {
				//console->raiseNMI();
			}
		}
		else if (scanline == 261) {
			//Clear vblank flag, sprite 0 hit, and overflow flags at end of vblank
			if (cycles == 1) {
				ppuStatus &= ~STATUS_VBL;
				ppuStatus &= ~STATUS_SPR0_HIT;
				ppuStatus &= ~STATUS_SPR_OVFLW;
			}
			//reset vertical offset at end of frame
			else if (cycles == 280) {
				//clear out the old vert data
				accessAddress &= 0x041F;
				//Masks out the vert data from the temp address and use it
				//to set the access address
				accessAddress |= temporaryAddress & 0x7BE0;
			}
			else if (cycles >= 257 && cycles <= 320) {
				spriteMemAddress = 0;
			}
			else if (cycles >= 321 && cycles <= 336) {
				fetchBGTile();
			}
		}
		incrementCycle();
		return;
	}

	//Code for rendering period

	//Do nothing if rendering is disabled
	if (! ( (ppuControl2 & CONTROL2_BG_RNDR) || (ppuControl2 & CONTROL2_SPR_RNDR) ) ) {
		incrementCycle();
		return;
	}


	//Memory fetch code

	//Memory fetches for BG tiles
	if (ppuControl2 & CONTROL2_BG_RNDR) {
		if ((cycles >= 1 && cycles <= 256) || (cycles >= 321 && cycles <= 336)) {
			fetchBGTile();
		}
		//Increment Y at end of bg fetches for this scanline
		if (cycles == 256) {
			incrementVertical();
		}
		//reset horizontal offset at end of bg fetches for this scanline
		else if (cycles == 257) {
			//clear out old horizontal data
			accessAddress &= 0x7BE0;
			//Mask out the horizontal data from the temp address and use it
			//to set the access address
			accessAddress |= temporaryAddress & 0x041F;
		}
	}

	//Memory fetches for sprites
	//(Sprite evaluation is only disabled when rendering is completely
	//disabled, if BG rendering is enabled, disabling sprite rendering only
	//hides the sprites, but they are still evaluated)
	if ((ppuControl2 & CONTROL2_SPR_RNDR) || (ppuControl2 & CONTROL2_BG_RNDR)) {
		if ((cycles >= 257 && cycles <= 320)) {
			fetchSpriteData();
		}
	}

	//END Memory fetch code

	//Sprite evaluation

	//First 64 cycles spent clearing out secondary OAM
	if (cycles >= 1 && cycles <= 64) {
		if (cycles % 2 == 0) {
			oamSecondary[cycles/2 - 1] = 0xFF;
		}
	}
	//Cycles 65-256: sprite evaluation
	//NOTE: technically reads are supposed to happen on odd cycles and
	//writes are supposed to happen on even cycles. To make things a little
	//simpler, I made it all happen on the even cycles. Pretty sure this won't
	//cause any problems, you'd have to be real fiddly about timing to make use
	//of this behavior. However, if there are problems with sprite evaluation
	//this is something to check
	else if (cycles >= 65 && cycles <= 256) {
		if (cycles % 2 == 0) {
			//I can't tell if I hate this or not, maybe it's just because it's so late at night
			//It's just a check that says if we've run through all of the OAM (becuase OAM is
			//256 bytes) then we're done evaluating sprites for this scanline so do nothing
			if (spriteMemAddress >= 256) {
				//Do nothing
			}
			//This is the part that evaluates sprites to be added to the secondary OAM
			else if (spriteIndex < 8) {
				loadSprites();
			}
			//Buggy overflow check
			else {
				checkSpriteOverflow();
			}
		}
	}

	//END Sprite evaluation code

	//Pixel rendering code

	//Calculate pixels and place in buffer
	if (cycles >= 1 && cycles <= 256) {
		calculatePixel();
	}

	if (cycles > 0 && cycles < 337) {
		//Shift registers
		patternShift0 >>= 1;
		patternShift1 >>= 1;

		//every 8 cycles load shift registers from buffers
		if ((cycles >= 1 && cycles <= 256) || (cycles >=321 && cycles <= 336)) {
			if (cycles % 8 == 0) {
				patternShift0 |= patternBuffer0 << 8;
				patternShift1 |= patternBuffer1 << 8;
				attrLatch = attrLatchBuffer;
			}
		}		
		
		attrShift0 >>= 1;
		attrShift0 |= (attrLatch & 0x01) << 7;
		attrShift1 >>= 1;
		attrShift1 |= (attrLatch & 0x02) << 6;
	}

	//Write pixels to video output 3 frames after calculation
	if (cycles >= 4 && cycles <= 259) {
		frame.buffer[cycles-4][scanline] = pixelBuffer[cycles-4];
	}

	//END Rendering code

	//End of cycle cleanup

	//The the sprite memory pointer is set to 0 in each tick
	//during the sprite tile loading process
	if (cycles >= 257 && cycles <= 320) {
		spriteMemAddress = 0;
	}

	//Clean up the rest of the sprite global variables at end of scanline
	if (cycles == 340) {
		spriteIndex = 0;
		sprite0Tracker >>= 1;
		readingSprite = 0;
	}

	incrementCycle();
}

bool PPU::isRendering() {
	//		Rendering is enabled  AND    We are in the rendering period     OR the pre-render line
	return (ppuControl2 & 0x18)   &&   ( (scanline >= 0 && scanline <= 239) || scanline == 261 );
}

//After reads/writes to PPUDATA, accessAddress is incremented
void PPU::incrementVAddress() {
	//Amount to increment by is determined by CONTROL1_INC
	if (ppuControl1 & CONTROL1_INC == 0x00) {
		accessAddress += 0x0001;
	}
	else { //if CONTROL1_INC is set
		accessAddress += 0x0010;
	}
}

//Determines whether the given address points to palette memory
//Used primarily to determine whether to use buffering behaviour
//for PPUDATA read
bool PPU::isPaletteMemory(uint16_t address) {
	return (address & 0x3FFF) < 0x3F00;
}

PPU::PPU(uint8_t *mem, uint8_t *oam) {
	vram = mem;

	paletteRAM = (uint8_t *) malloc(32);

	this->oam = oam;

	oamSecondary = (uint8_t *) malloc(64);

	ppuControl1 = 0x90;
	ppuControl2 = 0x18;
	ppuStatus = 0xA0;
	readBuffer = 0x00;
	registerLatch = 0x00;
	scanline = 261;
	cycles = 321;
	frameParity = 0;
	accessAddress = 0x0000;
	temporaryAddress = 0x0000;
	fineX = 0;
	writeToggle = 0;
	spriteIndex = 0;
	spriteMemAddress = 0x00;
	patternShift0 = 0x0000;
	patternShift1 = 0x0000;
	patternBuffer0 = 0x00;
	patternBuffer1 = 0x00;
	attrShift0 = 0x00;
	attrShift1 = 0x00;
	attrLatch = 0x00;
	attrLatchBuffer = 0x00;
	currentPattern = 0x00;
	sprite0Tracker = 0;
	resetCountdown = RESET_COUNTDOWN_START;

	for (int i = 0; i < 16; i++)
		spriteShift[i] = 0x00;
	for (int i = 0; i < 8; i++) {
		spriteAttr[i] = 0x00;
		spriteXCounter[i] = 0x00;
	}

	for (int i = 0; i < 256; i++)
		for (int j = 0; j < 240; j++)
			frame.buffer[i][j] = 0x3F;
}

PPU::~PPU() {
	delete paletteRAM;
}

//TODO: NMI code for PPUCTRL write
//writes data to register specified by regAddr
void PPU::writeRegister(uint8_t regAddr, uint8_t data) {
	//Access to PPU registers occurs through a latch which holds
	//the last value read/written, the various aspects of how this
	//affects execution will be explained as they come up in code
	//
	//Writing data to any PPU register fills the latch with the data written
	registerLatch = data;

	if (regAddr == PPUCTRL) {
		//writes to PPUCTRL ignored for a period after powerup/reset
		if (resetCountdown == 0) {
			ppuControl1 = data;

			//clear out old nametable data;
			temporaryAddress &= 0x0C00;
			//replace with new nametable data
			temporaryAddress |= (data & 0x03) << 10;

			//If PPUSTATUS vblank is still set and PPUCONTROL raiseNMI is toggled on, NMI is raised immediately
			if (ppuStatus & STATUS_VBL && ppuControl1 & CONTROL1_VBL) {
				//console->raiseNMI();
			}
		}
	}
	else if (regAddr == PPUMASK) {
		//writes to PPUMASK ignored for a period after powerup/reset
		if (resetCountdown == 0) {
			ppuControl2 = data;
		}
	}
	else if (regAddr == PPUSTATUS) {
		//PPUSTATUS is read-only
	}
	else if (regAddr == OAMADDR) {
		if (!isRendering()) {
			spriteMemAddress = data;
		}
	}
	else if (regAddr == OAMDATA) {
		if (!isRendering()) {
			oam[spriteMemAddress & 0xFF] = data;
			spriteMemAddress++;
			if (spriteMemAddress > 255)
				spriteMemAddress = 0;
		}
	}
	else if (regAddr == PPUSCROLL) {
		//writes to PPUSCROLL ignored for a period after powerup/reset
		if (resetCountdown == 0) {
			//Writing horizontal scroll
			if (writeToggle == 0) {
				//clear out coarse x from t and fill with coarse x from data
				temporaryAddress &= 0x001F;
				temporaryAddress |= (0xF8 & data) >> 3;
				//new fine x assignment
				fineX = data & 0x07;
				//Toggle w
				writeToggle= 1;
			}
			//Writing vertical scroll
			else if (writeToggle == 1) {
				//clearing out vertical scroll from t
				temporaryAddress &= 0x0C1F;
				//Setting coarse y
				temporaryAddress |= (data & 0xF8) << 2;
				//setting fine y
				temporaryAddress |= (data & 0x07) << 12;
				//toggle w
				writeToggle = 0;
			}
		}
	}
	else if (regAddr == PPUADDR) {
		//writes to PPUADDR ignored for a period after powerup/reset
		if (resetCountdown == 0) {
			//write high byte
			if (writeToggle == 0) {
				//clearing out high byte
				temporaryAddress &= 0x00FF;
				//setting new high byte
				temporaryAddress |= (data & 0x3F) << 8;
				//write toggle
				writeToggle = 1;
			}
			//write low byte and setting accessAddress
			else if (writeToggle == 1) {
				//clearing out low byte
				temporaryAddress &= 0xFF00;
				//setting new low byte
				temporaryAddress |= data;
				//toggle w
				writeToggle = 0;
				//set accessaddress
				accessAddress = temporaryAddress;
			}
		}
	}
	else if (regAddr == PPUDATA) {
		//PPUDATA access is only strictly legal outside of rendering
		//Access during rendering causes accessAddress to increment
		//in a pathological way and can result in a random location
		//being accessed. Because of this, writes during rendering are
		//ignored (though accessAddress is still incremented because there
		//are 2 games which make use of this for mid-frame graphics trickery).
		if (!isRendering()) {
			writeVRAM(accessAddress, data);
			incrementVAddress();
		}
		else { //access during rendering
			//write ignored
			//pathologically increment by performing a coarse x increment and a y increment
			incrementHorizontal();
			incrementVertical();
		}
	}
}

//reads data from register specified by regAddr
uint8_t PPU::readRegister(uint8_t regAddr) {
	//Reading any readable port will fill the latch with the bits read
	if (regAddr == PPUSTATUS) {
		//only the 3 highest bits of PPUSTATUS exist in hardware, so only
		//the 3 highest bits of the latch are affected
		registerLatch &= 0x1F;
		registerLatch |= ppuStatus & 0xE0;
	}
	else if (regAddr == OAMDATA) {
		registerLatch = oam[spriteMemAddress & 0xFF];
	}
	else if (regAddr == PPUDATA) {
		//Buffered non-palette read
		if (!isPaletteMemory(accessAddress)) {
			registerLatch = readBuffer;
			readBuffer = readVRAM(accessAddress);
			if (!isRendering())
				incrementVAddress();
			else {
				//pathological increment during rendering
				incrementHorizontal();
				incrementVertical();
			}
		}
		else { //Palette data read
			registerLatch = readVRAM(accessAddress);
		}
	}
	//Reading a write-only register returns the current value of the latch
	return registerLatch;
}

bool PPU::endOfFrame() {
	return frameEnd;
}

Frame PPU::getFrame() {
	return frame;
}