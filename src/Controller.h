#ifndef CONTROLLER_H
#define CONTROLLER_H

#define BTN_A 		0x01
#define BTN_B 		0x02
#define BTN_SELECT 	0x04
#define BTN_START 	0x08
#define BTN_UP		0x10
#define BTN_DOWN	0x20
#define BTN_LEFT	0x40
#define BTN_RIGHT	0x80


class Controller {

uint8_t inputState;
uint8_t readRegister;

bool strobe;

public:
	Controller() {
		inputState = 0x00;
		readRegister = 0x00;
		strobe = false;
	}

	void strobeHigh() {
		strobe = true;
	}

	void strobeLow() {
		if (strobe)
			readRegister = inputState;
		strobe = false;
	}

	uint8_t read() {
		uint8_t ret = readRegister & 0x01;
		readRegister >>= 1;
		return ret;
	}

	void pressButton(uint8_t key) {
		inputState |= key;
	}

	void releaseButton(uint8_t key) {
		inputState &= ~key;
	}
};

#endif