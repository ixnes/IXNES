#include "SDL2/SDL.h"
#include <iostream>
#include <cstdint>
#include <cmath>

#include "ROM.h"
#include "Console.h"
#include "PPU.h"
#include "Controller.h"

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 240
#define SCREEN_FPS 60
#define SCREEN_TICKS_PER_FRAME (1000/SCREEN_FPS)

#define PI 3.14159265

using namespace std;

int main(int argc, char *argv[]) {
	                     //00		   01		   02		   03	
	uint32_t palette[] = { 0x656565FF, 0x002D69FF, 0x131F7FFF, 0x3C137CFF,
						 //04		   05		   06		   07
						   0x600B62FF, 0x730A37FF, 0x710F07FF, 0x5A1A00FF,
						 //08		   09		   0A 		   0B
						   0x342800FF, 0x0B3400FF, 0x003C00FF, 0x003D10FF,
						 //0C 		   0D 		   0E		   0F
						   0x003840FF, 0x000000FF, 0x000000FF, 0x000000FF,
						 //10		   11		   12		   13	
						   0xAEAEAEFF, 0x0F63B3FF, 0x4051D0FF, 0x7841CCFF,
						 //14		   15		   16		   17
						   0xA736A9FF, 0xC03470FF, 0xBD3C30FF, 0x9F4A00FF,
						 //18		   19		   1A 		   1B
						   0x6D5C00FF, 0x366D00FF, 0x077704FF, 0x00793DFF,
					     //1C 		   1D 		   1E		   1F
						   0x00727DFF, 0x000000FF, 0x000000FF, 0x000000FF,
						 //20		   21		   22		   23	
						   0xFEFEFFFF, 0x5DB3FFFF, 0x8FA1FFFF, 0xC890FFFF,
						 //24		   25		   26		   27
						   0xF785FAFF, 0xFF83C0FF, 0xFF8B7FFF, 0xEF9A49FF,
						 //28		   29		   2A 		   2B
						   0xBDAC2CFF, 0x85BC2FFF, 0x55C753FF, 0x3CC98CFF,
					     //2C 		   2D 		   2E		   2F
						   0x3EC2CDFF, 0x4E4E4EFF, 0x000000FF, 0x000000FF,
						 //30		   31		   32		   33	
						   0xFEFEFEFF, 0xBCDFFFFF, 0xD1D8FFFF, 0xE8D1FFFF,
						 //34		   35		   36		   37
						   0xFBCDFDFF, 0xFFCCE5FF, 0xFFCFCAFF, 0xF8D5B4FF,
						 //38		   39		   3A 		   3B
						   0xE4DCA8FF, 0xCCE3A9FF, 0xB9E8B8FF, 0xAEE8D0FF,
					     //3C 		   3D 		   3E		   3F
						   0xAFE5EAFF, 0xB6B6B6FF, 0x000000FF, 0x000000FF};

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		cout << "Error: failed to initialize SDL" << endl;
	}

	SDL_Window *window = SDL_CreateWindow("IXNES", 
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
			SCREEN_WIDTH*3, SCREEN_HEIGHT*3, 0);

	if (!window) {
		cout << "Error: failed to create window" << endl;
	}

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	if (!renderer) {
		cout << "Error: failed to create renderer" << endl;
	}

	SDL_Surface *testImage = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 
								0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

	if (!testImage) {
		cout << "Error: failed to create surface for image" << endl;
	}

	uint32_t (*pixels)[SCREEN_WIDTH] = (uint32_t (*)[SCREEN_WIDTH]) testImage->pixels;

	Frame frame;

	RomImage rom(argv[1], false);

	Console con(&rom);

	Controller *controller = con.getController1();

	int framecounter = 0;

	bool keep_window_open = true;
	while (keep_window_open) {
		unsigned int start = SDL_GetTicks();

		uint8_t buttonPress = 0x00;
		uint8_t buttonRelease = 0x00;

		SDL_Event e;
		while (SDL_PollEvent(&e) > 0) {
			if (e.type == SDL_QUIT) {
				keep_window_open = false;
			}
			else if (e.type == SDL_KEYDOWN) {
				switch (e.key.keysym.sym) {
					case SDLK_UP:
						buttonPress |= BTN_UP;
						break;
					case SDLK_DOWN:
						buttonPress |= BTN_DOWN;
						break;
					case SDLK_LEFT:
						buttonPress |= BTN_LEFT;
						break;
					case SDLK_RIGHT:
						buttonPress |= BTN_RIGHT;
						break;
					case SDLK_z:
						buttonPress |= BTN_B;
						break;
					case SDLK_x:
						buttonPress |= BTN_A;
						break;
					case SDLK_RETURN:
						buttonPress |= BTN_START;
						break;
					case SDLK_RSHIFT:
						buttonPress |= BTN_SELECT;
						break;
				}
			}
			else if (e.type == SDL_KEYUP) {
				switch (e.key.keysym.sym) {
					case SDLK_UP:
						buttonRelease |= BTN_UP;
						break;
					case SDLK_DOWN:
						buttonRelease |= BTN_DOWN;
						break;
					case SDLK_LEFT:
						buttonRelease |= BTN_LEFT;
						break;
					case SDLK_RIGHT:
						buttonRelease |= BTN_RIGHT;
						break;
					case SDLK_z:
						buttonRelease |= BTN_B;
						break;
					case SDLK_x:
						buttonRelease |= BTN_A;
						break;
					case SDLK_RETURN:
						buttonRelease |= BTN_START;
						break;
					case SDLK_RSHIFT:
						buttonRelease |= BTN_SELECT;
						break;
				}
			}
		}
		

		//Grab the frame
		frame = con.getFrame();

		//Translate frame data to pixel data
		for (int j = 0; j < SCREEN_HEIGHT; j++) {
			for (int i = 0; i < SCREEN_WIDTH; i++) {
				pixels[j][i] = palette[frame.buffer[i][j]];
			}
		}

		//Update controller status
		controller->pressButton(buttonPress);
		controller->releaseButton(buttonRelease);
		
		//Render frame on screen
		SDL_RenderClear(renderer);

		SDL_Texture *txt = SDL_CreateTextureFromSurface(renderer, testImage);

		SDL_RenderCopy(renderer, txt, NULL, NULL);

		SDL_RenderPresent(renderer);

		SDL_DestroyTexture(txt);

		//cout << "Frame " << framecounter << " rendered" << endl;

		//framecounter++;

		//Get frame time and delay to cap framerate
		unsigned int frameTime = SDL_GetTicks() - start;

		if (frameTime < SCREEN_TICKS_PER_FRAME) {
			SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTime);
		}
	}

	//Free resources
	SDL_FreeSurface(testImage);

	SDL_DestroyRenderer(renderer);

	SDL_DestroyWindow(window);

	SDL_Quit();
}