/******************************************************************************
main.c
Jos8: a work in progress CHIP-8 emulator using C and SDL2
(c) 2018 Jos van Mourik

Based on multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter
******************************************************************************/

// includes
#include <stdio.h>
#include <stdlib.h>
#include "chip8.h"
#include "SDL2/SDL.h"

// defines
#define SCALE 10

// SDL snancode to CHIP-8 keycode
const int keytype[16] = 
{
	SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, 
	SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A,
	SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C, 
	SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V
};

// put graphics on command line				
void debug_gxf(void)
{
	for(int y = 0; y < 32; y++)
	{
		for(int x = 0; x < 64; x++)
		{
			if(screen[x][y]) printf("*");
			else printf(" ");
		}
		printf("\n");
	}
	system("cls");
}

// main emulator loop
int main(int argc, char *argv[]) 
{
	chip8_init();
	
	// running state
    unsigned char running = 1;
    
    // load ROM if file exists
	if (chip8_load("INVADERS")) running = 0;
	
    // initialize SDL
    SDL_Init(SDL_INIT_VIDEO);
    
    // initialize SDL window
	SDL_Window *window = SDL_CreateWindow("Jos8", SDL_WINDOWPOS_UNDEFINED, 
						 SDL_WINDOWPOS_UNDEFINED, 64*SCALE, 32*SCALE, SDL_WINDOW_OPENGL);

    // initialize SDL renderer
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	
	// scale SDL renderer
	SDL_RenderSetScale(renderer, SCALE, SCALE);

	// SDL scankey pointer
	const Uint8 *state = SDL_GetKeyboardState(NULL);

	// SDL even union
    SDL_Event event;

    while(running)
    {
		// process events
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
            {
                running = 0;
            }
        }
        
        // read keys into array
        for(int k = 0; k < 16; k++)
        {
        	key[k] = state[keytype[k]];
		}
        
        // run cpu cycle and draw if screen flag is set
		if(chip8_cycle() == 1)
		{
			for(int y = 0; y < 32; y++)
			{			
				for(int x = 0; x < 64; x++)
				{
					if(screen[x][y]) // white pixel
					{
						SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
						SDL_RenderDrawPoint(renderer, x, y);
					}
					else // black pixel
					{
						SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
						SDL_RenderDrawPoint(renderer, x, y);
					}
				}					
			}
			SDL_RenderPresent(renderer); // update screen
		}
		SDL_Delay(2); // 500hz
	}

	// release SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

