#include "Console.h"
#include "ROM.h"

int main(int argc, char *argv[]) {
	RomImage rom(argv[1], false);

	Console con(&rom);

	con.debug();
}