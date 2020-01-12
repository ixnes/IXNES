//Nothing gives me more joy than taking something apart, figuring out how it works and putting it back together
//But there's a problem with that when it comes to people
//Even when you're trying to take one apart from the inside, perhaps especially so then
//Because the harder you try to pry something open, the tighter it shuts on you
//There's an element of approaching the divine: you do so on it's terms
//In trying to find who you are you must agree to submit to whoever that is
//You must respect it before it will agree to reveal itself, before it will allow you to inspect it
//Because there is a terrible intimacy to being seen, even by ones own self
//So taking apart a thing is entirely different from taking apart a person
//Because a thing is dead
//A thing yields itself to be examined without argument
//But a person fights back
//And to see who they are they have to agree to show you
//They have to feel safe
//And if you don't feel safe with yourself you'll never know who you are

#ifndef PPU_H
#define PPU_H

#include <cstdint>

#define CONTROL1_NT_X		0x01 //X Scroll name table selection
#define CONTROL1_NT_Y		0x02 //Y scroll name table selection
#define CONTROL1_INC		0x04 //Controls VRAM address increment on $2007 access
									//0 - increment 1
									//1 - increment 32
#define CONTROL1_SPR_PT		0x08 //Identifies which pattern table sprites are stored in
									//0 - $0000
									//1 - $1000
#define CONTROL1_BG_PT		0x10 //Identifies which pattern table background tiles are stored in (same as SPR_PT)
#define CONTROL1_SPR_SIZE	0x20 //Controls sprite size
									//0 - 8x8 pixels
									//1 - 8x16 pixels
#define CONTROL1_MSTR_SLV	0x40 //Switches PPU between master and slave mode (unused)
#define CONTROL1_VBL		0x80 //Controls whether NMI occurs on vblank (disable when 0)

#define CONTROL2_COLOR		0x01 //Controls whether system is in color or monochome mode
									//0 - color
									//1 - monochrome
#define CONTROL2_CLIP_LEFT	0x02 //Controls whether the left 8 pixels are clipped
									//0 - clipped
									//1 - shown
#define CONTROL2_CLIP_RGHT	0x04 //Controls whether the right 8 pixels are clipped (same as CLIP_LEFT)
#define CONTROL2_BG_RNDR	0x08 //Controls whether the background is rendered
									//0 - background rendering off
									//1 - background rnedering on
#define CONTROL2_SPR_RNDR	0x10 //Controls wheter sprites are rednered (same as BG_RNDR)
#define CONTROL2_TINT_R		0x20 //Controls red tint
#define CONTROL2_TINT_G		0x40 //Controls green tint
#define CONTROL2_TINT_B		0x80 //Controls blue tint

//Conflicting information on what PPUSTATUS bit 4 does
//One source says if it is set then writes to VRAM should be ignored,
//but nothing else corroborates this and what exactly that means is unclear

#define STATUS_SPR_OVFLW	0x20 //Set if more than 8 sprites on a scanline
#define STATUS_SPR0_HIT		0x40 //Set if non-transparent pixel of sprite 0 overlaps a nontransparent background pixel
#define STATUS_VBL			0x80 //Set during vblank

//See definition of 'resetCountdown'
#define RESET_COUNTDOWN_START	29658*3

//Definitions for the address of each PPU register
#define PPUCTRL 	0
#define PPUMASK		1
#define PPUSTATUS 	2
#define OAMADDR		3
#define OAMDATA		4
#define PPUSCROLL	5
#define PPUADDR		6
#define PPUDATA		7

//Forward declaration of Console class
class Console;

typedef struct Frame Frame;

struct Frame {
	uint8_t buffer[256][240];
};

class PPU {
private:
	uint8_t *vram;

	//Registers used by the CPU to program the PPU
	//see definitions at top for information on what each bit does
	uint8_t ppuControl1;
	uint8_t ppuControl2;
	uint8_t ppuStatus;

	//Reads to PPUDATA ($2007) are buffered. The buffer is updated with the value
	//at accessAdress immediately following a read from PPUDATA
	uint8_t readBuffer;

	//See readRegister and writeRegister definitions
	uint8_t registerLatch;

	//pointer to memory region containing palette RAM 32 bytes
	uint8_t *paletteRAM;

	//256 byte object attribute memory
	//holds 64, 4-byte sprite attribute data
		//Byte 0 - y position of top of sprite - 1
		//Byte 1 - tile index number
		//Byte 2 - attributes
			//76543210
			//||||||++-pallete number of sprite
			//|||+++---unimplemented
			//||+------priority
			//|+-------flip sprite horizontally
			//+--------flip sprite vertically
		//Byte 3 - x position of left side of sprite
	uint8_t *oam;

	//Secondary OAM that loaded with 8 4-byte sprite entries for the next line
		//Byte 0 - Y offset within sprite calculated based on range comparison with scanline
			//NOTE: there's conflicting information on the wikis about Byte 0
			//of entries in secondary OAM. I'm going with this interpretation
			//because it makes the most sense to me and it shouldn't really matter
			//how this part is implemented
		//Byte 1 - tile index
		//Byte 2 - attributes
		//Byte 3 - x position
	uint8_t *oamSecondary;

	//Holds current scanline number
	//0-239		visible scanlines (rendering happens here)
	//240		idle postrender scanline
	//241-260	vblank lines (memory access from CPU is only safe here)
	//261		prerender scanline (this was to initialize the sprite rendering pipeline, for the purposes of this emulator it does nothing)
	int16_t scanline;

	//Holds cycle number within current scanline
	//Memory Fetch pipeline:
	//0			Idle cycle, skipped on scanline 0 during odd frames (one source indicates BG_RNDR must also be set, but nothing else corroborates this)
	//1-256		Memory fetch for background tiles for tiles 3-33 (if fine scroll is not zero, screen will hangover onto 33rd tile)
				//Vblank
				//SPR0_HIT set at cycle 2 at the earliest
				//first pixel rendered at cycle 4, one pixel rendered per cycle (256 pixels/scanline)
				//249-256 unused tile fetch cycle
	//65-256	sprite evaluation for next scanline
	//257-320	Memory fetch for sprite data
	//321-336	Memory fetch for tiles 1 and 2 of next scanline
	//337-340	Two unused nametable fetches
	//Sprite evaluation pipeline:
	//1-64		Clear secondary OAM to $FF at each location
	//65-256	Evaluating sprites
	//257-320	Memory fetch
	//321-340+0	Idle (in hardware these cycles were used to initalize the pipeline)
	int16_t cycles;

	//Keeps track of whether PPU is rendering and even or odd frame (see cycle comments)
	bool frameParity;

	//Holds the address in VRAM used for reading and writing
	uint16_t accessAddress;

	//Used to hold location of top left corner of screen during rendering
	//This lets the rendering loop know where to reset x/y in accessAddress
	//to at the end of each sca reduced phone-crashing power spikes, until…nline/frame
	uint16_t temporaryAddress;

	//3-bit register used to determine fine x scroll within tile
	uint8_t fineX;

	//Writes to $2005 and $2006 have different behavior based on the value
	//of this toggle. Writing to either of those registers toggles this bit
	//and reading $2002 resets it.
	bool writeToggle;

	//Used by the sprite evaluation pipeline to indicate how many
	//Sprites have already been located for the next scanline
	//There is a max of 8 sprites rendered per scanline, after which
	//if anymore are found the sprite overflow flag is set and all
	//further
	uint8_t spriteIndex;

	//Used by sprite eval pipeline to keep track of next OAM address to read
	uint16_t spriteMemAddress;

	//16 bit shift registers for use in rendering pipeline
	//Holds pattern table information, shifted right every cycle
	//See rendering code for in-depth explanation
	uint16_t patternShift0;
	uint16_t patternShift1;

	//Buffers for the pattern shift registers
	//Loaded by the memory fetch and loaded into the upper 8 bits
	//of the pattern shift registers every 8 cycles
	uint8_t patternBuffer0;
	uint8_t patternBuffer1;

	//8 bit shift registers for use in rendering pipeline
	//Holds attribute information, shifted right every cycle
	//See rendering code for more in-depth explanation
	uint8_t attrShift0;
	uint8_t attrShift1;	

	//Holds attribute data to be shifted into the attribute shift registers
	//reloaded every 8 cycles
	uint8_t attrLatch;

	//Buffer for attrLatch
	//Loaded by memory fetch and loaded into attrLatch every 8 cycles
	uint8_t attrLatchBuffer;

	//Holds retrieved pattern index byte for current tile fetch cycle
	uint8_t currentPattern;

	//Used for sprite part of rendering pipeline
	//Loaded during 
	uint8_t spriteShift[16];
	uint8_t spriteAttr[8];
	uint8_t	spriteXCounter[8];

	//Used to track whether sprite 0 is in the next scanline
	//	0x00 (0) - sprite 0 has not been detected in either the current or the next scanline
	//	0x01 (1) - sprite 0 is being rendered in the current scanline
	//	0x10 (2) - sprite 0 will be rendered in the next scanline
	//	0x11 (3) - sprite 0 is being rendered in the current scanline and the next scanline
	//
	//With this setup, when sprite evaluation finds sprite 0 in range, it can set bit 2,
	//when the hit conditions are met, the renderer can check bit 1, and at the end of
	//the scanline, this can be shifted right so that 'next line' becomes 'current line'
	uint8_t sprite0Tracker; 

	//Used to keep track of sprite evaluation state
	// 0 	- checking ranges
	// 1-3 	- found in-range sprite, copy data then decrement
	uint8_t readingSprite;

	//Buffer used to store rendering results during the 3 cycles between calculation and output
	uint8_t pixelBuffer[256];

	//Buffer used to store the pallette data for the frame
	Frame frame;

	//upon reset, certain registers cannot be written until ~29,658 cycles have passed
	//This variable counts down until that point has been reached
	uint16_t resetCountdown;

	//set at the end of the frame and reset at the beginning of the next cycle
	bool frameEnd;

	//This will get more complex later on when I implement mirroring
	//For now it's just a simple placeholder for maintainability's sake
	void writeVRAM(uint16_t address, uint8_t data);

	//Maintainability placeholder until I develop more sophisticated memory handling
	uint8_t readVRAM(uint16_t address);

	//Retrieves appropriate byte in name table based on rendering address
	//Bit manipulation based on how the accessAddress register is used
	//during rendering. See description of rendering (wherever I end up putting that)
	uint8_t retrieveNameTableByte(uint16_t address);

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
	uint8_t retrieveAttrTableBits(uint16_t address);

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
	uint8_t retrievePatternTableByte(bool patternTable, uint8_t patternByte, bool plane, uint8_t yOffset);

	//The following several functions were pulled out of the cycle function for readability
	//Halfway through inplementing cycle it become an unmanagable tangle of nested ifs

	void incrementHorizontal();

	void incrementVertical();

	void fetchBGTile();

	void fetchSpriteData();

	void loadSprites();

	void checkSpriteOverflow();

	bool isTransparent(uint8_t pixel);

	void calculatePixel();

	//Increments the cycle and scanline counters
	void incrementCycle();

	bool isRendering();

	//After reads/writes to PPUDATA, accessAddress is incremented
	void incrementVAddress();

	//Determines whether the given address points to palette memory
	//Used primarily to determine whether to use buffering behaviour
	//for PPUDATA read
	bool isPaletteMemory(uint16_t address);
public:

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
	void cycle();

	PPU(uint8_t *mem, uint8_t *oam);

	~PPU();

	//TODO: NMI code for PPUCTRL write
	//writes data to register specified by regAddr
	void writeRegister(uint8_t regAddr, uint8_t data);

	//reads data from register specified by regAddr
	uint8_t readRegister(uint8_t regAddr);

	bool endOfFrame();
	Frame getFrame();
};

#endif