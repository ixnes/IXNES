#include "APU.h"
#include "Console.h"
#include "Debug.h"

#include <iostream>

using namespace std;

APU::APU(Console *con) {
	console = con;

	sequenceMode = SEQMODE_4STEP;

	irqDisabled = true;

	step = 0;

	divider = DIVIDER_COUNT;

	channelsEnabled = 0x00;

	//Pulse channel 0
	pulseTimer0 = 0x00;

	pulseDuty0 = 0x00;

	pulseCounter0 = 0;

	pulseCounterHalt0 = false;

	pulseVolMode0 = VOL_ENVELOPE;
	
	pulseVol0 = 0x00;;

	pulseSweepEnabled0 = false;

	pulseDivider0 = 0x00;

	pulseNegate0 = false;

	pulseShift0 = 0x00;

	pulseReload0 = false;

	//Pulse channel 1
	pulseTimer1 = 0x0000;

	pulseDuty1 = 0x00;

	pulseCounter1 = 0;

	pulseCounterHalt1 = false;

	pulseVolMode1 = VOL_ENVELOPE;
	
	pulseVol1 = 0x00;

	pulseSweepEnabled1 = false;

	pulseDivider1 = 0x00;

	pulseNegate1 = false;

	pulseShift1 = 0x00;

	pulseReload1 = false;

	//Triangle channel	
	triTimer = 0x0000;

	triCounter = 0;

	triCounterHalt = false;

	triLinCounter = 0;

	triReload = false;

	//Noise channel
	noiseTimer = 0x0000;

	noiseCounter = 0;

	noiseCounterHalt = false;

	noiseVolMode = VOL_ENVELOPE;

	noiseVol = 0x00;

	noiseMode = false;

	noisePeriod = 0x0000;

	//DMC channel
	dmcIRQEnabled = false;

	dmcIRQ = false;

	dmcLoopFlag = false;

	dmcFreq = 0x0000;

	dmcOutput = 0x00;

	dmcAddress = 0x00;

	dmcSampleLength = 0x0000;


	irqSet = 0;

	frameIRQ = false;

	frameCounterReset = 0;

	writeFrameCounter(0x00);

	for (int i = 0; i < 4; i++)
		cycle();
}

void APU::clockChannelCounters() {
	//cout << "clock" << endl;
	if (pulseCounter0 != 0 && !pulseCounterHaltBuf0) {
		pulseCounter0--;

		#ifdef DBG_APU_COUNTERS
		cout << "Pulse 0 counter decremented to: " << pulseCounter0 << endl;
		#endif
	}
	if (pulseCounter1 != 0 && !pulseCounterHaltBuf1) {
		pulseCounter1--;

		#ifdef DBG_APU_COUNTERS
		cout << "Pulse 1 counter decremented to: " << pulseCounter0 << endl;
		#endif
	}
	if (triCounter != 0 && !triCounterHaltBuf) {
		triCounter--;

		#ifdef DBG_APU_COUNTERS
		cout << "Triangle counter decremented to: " << pulseCounter0 << endl;
		#endif
	}
	if (noiseCounter != 0 && !noiseCounterHaltBuf) {
		noiseCounter--;

		#ifdef DBG_APU_COUNTERS
		cout << "Noise counter decremented to: " << pulseCounter0 << endl;
		#endif
	}
}

void APU::setUpClock() {
	pulseCounterHaltBuf0 = pulseCounterHalt0;
	pulseCounterHaltBuf1 = pulseCounterHalt1;
	triCounterHaltBuf = triCounterHalt;
	noiseCounterHaltBuf = noiseCounterHalt;
}

//DEBUG
int cycleCounter = 0;

void APU::performStep() {
	#ifdef DBG_APU_FRAMECOUNTER
	cout << "step " << +step << " @ cycle " << cycleCounter << endl;
	#endif

	if (sequenceMode == SEQMODE_4STEP) {
		switch (step) {
			case 2:
				#ifdef DBG_APU_FRAMECOUNTER
				cout << "\tcounter clock" << endl;
				#endif
				clockWaiting = true;
				setUpClock();
				break;
			case 4:
				#ifdef DBG_APU_FRAMECOUNTER
				cout << "\tcounter clock" << endl;
				#endif
				clockWaiting = true;
				setUpClock();
				if (!irqDisabled) {
					frameIRQ = true;
					irqSet = 2;
					//console->raiseIRQ();
				}
				step = 0;
				break;
		}
	}
	else { //If in 5-step sequence mode
		switch (step) {
			case 2:
				clockWaiting = true;
				setUpClock();
				break;
			case 5:
				clockWaiting = true;
				setUpClock();
				step = 0;
				break;
		}
	}

	#ifdef DBG_APU_FRAMECOUNTER
	cout << "Frame IRQ " << ((frameIRQ) ? "set" : "clear") << endl;
	#endif
}

void APU::cycle() {
	if (irqSet) {
		irqSet--;
		if (!irqDisabled) {
			frameIRQ = true;
			
			#ifdef DBG_APU_FRAMECOUNTER
			cout << "Set frame IRQ, cycle " << dec << cycleCounter << endl;
			#endif
		}
	}

	//Main apu loop
	if (cycleSwitch) {
		cycleCounter++;

		if (frameCounterReset > 0) {
			frameCounterReset--;
			if (frameCounterReset == 0) {
				divider = DIVIDER_COUNT;
				if (sequenceMode == SEQMODE_5STEP) {
					setUpClock();
					clockChannelCounters();
				}
				step = 0;

				cycleCounter = 0;
			}
		}

		if (divider == 0) {
			step++;
			if (step < 2)
				divider = DIVIDER_COUNT;
			else if (step == 4 && sequenceMode == SEQMODE_5STEP)
				divider = DIVIDER_COUNT - 2;
			else
				divider = DIVIDER_COUNT + 1;
			performStep();
		}

		divider--;
	}
	//1 CPU delay for half and quarter frame clocks
	else {
		if (clockWaiting) {
			clockChannelCounters();
			clockWaiting = false;
		}
	}
	cycleSwitch = !cycleSwitch;
}

void APU::writeRegister(uint8_t regAddr, uint8_t data) {
	#ifdef DBG_APUREG_ACCESS
	cout << "Register " << +regAddr << " write" << endl;
	cout << "\tData: " << hex << +data << endl;
	#endif

	switch (regAddr) {
		case 0x00:
			pulseDuty0 = data >> 6;
			pulseCounterHalt0 = data & 0x20;
			pulseVolMode0 = data & 0x10;
			pulseVol0 = data & 0x0F;
			break;
		case 0x01:
			pulseSweepEnabled0 = data & 0x80;
			pulseDivider0 = (0x70 & data) >> 4;
			pulseNegate0 = data & 0x08;
			pulseShift0 = data & 0x07;
			break;
		case 0x02:
			pulseTimer0 &= 0xFF00;
			pulseTimer0 |= data;
			break;
		case 0x03:
			pulseTimer0 &= 0x00FF;
			pulseTimer0 |= (data & 0x07) << 8;
			if (clockWaiting && (pulseCounter0 == 0))
				pulseCounterHaltBuf0 = true;
			if ((channelsEnabled & 0x01) && !(clockWaiting && (pulseCounter0 > 0)))
				pulseCounter0 = lengthTable[data >> 3];
			break;
		case 0x04:
			pulseDuty1 = data >> 6;
			pulseCounterHalt1 = data & 0x20;
			pulseVolMode1 = data & 0x10;
			pulseVol1 = data & 0x0F;
			break;
		case 0x05:
			pulseSweepEnabled1 = data & 0x80;
			pulseDivider1 = (0x70 & data) >> 4;
			pulseNegate1 = data & 0x08;
			pulseShift1 = data & 0x07;
			break;
		case 0x06:
			pulseTimer1 &= 0xFF00;
			pulseTimer1 |= data;
			break;
		case 0x07:
			pulseTimer1 &= 0x00FF;
			pulseTimer1 |= (data & 0x07) << 8;
			if (clockWaiting && (pulseCounter1 == 0))
				pulseCounterHaltBuf1 = true;
			if ((channelsEnabled & 0x02) && !(clockWaiting && pulseCounter1 > 0))
				pulseCounter1 = lengthTable[data >> 3];
			break;
		case 0x08:
			triCounterHalt = data & 0x80;
			triLinCounter = data & 0x7F;
			break;
		case 0x0A:
			triTimer &= 0xFF00;
			triTimer |= data;
			break;
		case 0x0B:
			triTimer &= 0x00FF;
			triTimer |= (data & 0x07) << 8;
			if (clockWaiting && (triCounter == 0))
				triCounterHaltBuf = true;
			if ((channelsEnabled & 0x04) && !(clockWaiting && triCounter > 0))
				triCounter = lengthTable[data >> 3];
			break;
		case 0x0C:
			noiseCounterHalt = data & 0x20;
			noiseVolMode = data & 0x10;
			noiseVol = data & 0x0F;
			break;
		case 0x0E:
			noiseMode = data & 0x80;
			noisePeriod = data & 0x0F;
			break;
		case 0x0F:
			if (clockWaiting && (noiseCounter == 0))
				noiseCounterHaltBuf = true;
			if ((channelsEnabled & 0x08) && !(clockWaiting && noiseCounter > 0))
				noiseCounter = lengthTable[data >> 3];
			break;
		case 0x10:
			dmcIRQEnabled = data & 0x80;
			dmcLoopFlag = data & 0x40;
			dmcFreq = data & 0x0F;
			break;
		case 0x11:
			dmcOutput = data & 0x7F;
			break;
		case 0x12:
			dmcAddress = data;
			break;
		case 0x13:
			dmcSampleLength = (16 * data) + 1;
			break;
	}
}

void APU::writeControl(uint8_t data) {
	#ifdef DBG_APUREG_ACCESS
	cout << "Control register write" << endl;
	cout << "\tData: " << hex << +data << endl;
	#endif

	channelsEnabled = data & 0x1F;

	if (data & 0x08) {
		noiseCounter = 0;
	}
	if (data & 0x04) {
		triCounter = 0;
	}
	if (data & 0x02) {
		pulseCounter1 = 0;
	}
	if (data & 0x01) {
		pulseCounter0 = 0;
	}
}

uint8_t APU::readStatus() {
	#ifdef DBG_APUREG_ACCESS
	cout << "Status register read" << endl;
	#endif

	uint8_t ret;

	if (pulseCounter0 > 0)
		ret |= 0x01;
	if (pulseCounter1 > 0)
		ret |= 0x02;
	if (triCounter > 0)
		ret |= 0x04;
	if (noiseCounter > 0)
		ret |= 0x04;

	if (frameIRQ)
		ret |= 0x40;

	frameIRQ = false;

	#ifdef DBG_APUREG_ACCESS
	cout << "\tData: " << hex << +ret << endl;
	#endif

	return ret;
}

void APU::writeFrameCounter(uint8_t data) {
	#ifdef DBG_APUREG_ACCESS
	cout << "Frame counter register write" << endl;
	cout << "\tData: " << hex << +data << endl;
	#endif

	sequenceMode = data & 0x80;
	irqDisabled = data & 0x40;

	if (irqDisabled)
		frameIRQ = false;

	frameCounterReset = 2;
}

bool APU::irqWaiting() {
	return frameIRQ;
}