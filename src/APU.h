#ifndef APU_H
#define APU_H

#include <cstdint>

#define SEQMODE_4STEP 0x00
#define SEQMODE_5STEP 0x01

//#define DIVIDER_COUNT 3728
#define DIVIDER_COUNT 3728

#define VOL_ENVELOPE 0x00
#define VOL_CONSTANT 0x20

class Console;

/////////////////////////////////////////////////////////
//
//	Right now all that is implemented is the frame IRQ
//
/////////////////////////////////////////////////////////

class APU {
private:
	Console *console;
						         //0    1	2   3   4   5	6	7	 8	 9	 A 	 B 	 C 	 D 	 E 	 F
	uint8_t lengthTable[0x20] = { 10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
							 	  12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

	bool sequenceMode;

	bool irqDisabled;

	int8_t step;

	int32_t divider;

	uint8_t channelsEnabled;

	bool cycleSwitch;

	bool clockWaiting;

	bool pulseCounterHaltBuf0;
	bool pulseCounterHaltBuf1;
	bool triCounterHaltBuf;
	bool noiseCounterHaltBuf;

	//Pulse channel 0
	uint16_t pulseTimer0;

	uint8_t pulseDuty0;

	int16_t pulseCounter0;

	bool pulseCounterHalt0;

	bool pulseVolMode0;
	
	uint8_t pulseVol0;

	bool pulseSweepEnabled0;

	uint8_t pulseDivider0;

	bool pulseNegate0;

	uint8_t pulseShift0;

	bool pulseReload0;

	//Pulse channel 1
	uint16_t pulseTimer1;

	uint8_t pulseDuty1;

	int16_t pulseCounter1;

	bool pulseCounterHalt1;

	bool pulseVolMode1;
	
	uint8_t pulseVol1;

	bool pulseSweepEnabled1;

	uint8_t pulseDivider1;

	bool pulseNegate1;

	uint8_t pulseShift1;

	bool pulseReload1;

	//Triangle channel	
	uint16_t triTimer;

	int16_t triCounter;

	bool triCounterHalt;

	int16_t triLinCounter;

	bool triReload;

	//Noise channel
	uint16_t noiseTimer;

	int16_t noiseCounter;

	bool noiseCounterHalt;

	bool noiseVolMode;

	uint8_t noiseVol;

	bool noiseMode;

	uint16_t noisePeriod;

	//DMC channel
	bool dmcIRQEnabled;

	bool dmcIRQ;

	bool dmcLoopFlag;

	uint16_t dmcFreq;

	uint8_t dmcOutput;

	uint8_t dmcAddress;

	uint16_t dmcSampleLength;


	int8_t irqSet;

	bool frameIRQ;

	int16_t frameCounterReset;

	void performStep();

public:

	APU(Console *con);

	void setUpClock();

	void clockChannelCounters();

	void cycle();

	void writeRegister(uint8_t regAddr, uint8_t data);

	void writeControl(uint8_t data);

	uint8_t readStatus();

	void writeFrameCounter(uint8_t data);

	bool irqWaiting();
};

#endif