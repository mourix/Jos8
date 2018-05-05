/******************************************************************************
main.c
Jos8: a work in progress CHIP-8 emulator using C and SDL2
(c) 2018 Jos van Mourik

Based on multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter
******************************************************************************/

// includes
#include <stdio.h>
#include <stdbool.h>
#include "chip8.h"
#include "SDL2/SDL.h"

// scaling of 64x32 CHIP-8 screen
#define SCALE 10

// SDL snancode to CHIP-8 keycode conversion
const int keyconvert[16] = 
{
	SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, 
	SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A,
	SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C, 
	SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V
};

// render a full frame
void render_frame(SDL_Renderer *renderer)
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

// main emulator loop
int main(int argc, char *argv[]) 
{
    bool running = false; // running state
    bool paused = false; // pause state
    bool debug = false; // debug state
    
    // load ROM if file argument exists
	if(argc == 2) 
	{
		chip8_init();
		if(!chip8_load(argv[1])) // don't run if file can't be openened
		{
			running = true;
		}
	}
	else printf("Usage: Jos8 [romname]");
	
    // setup SDL
    SDL_Init(SDL_INIT_VIDEO); 
	SDL_Window *window = SDL_CreateWindow("Jos8", SDL_WINDOWPOS_UNDEFINED, 
						 SDL_WINDOWPOS_UNDEFINED, 64*SCALE, 32*SCALE,
						 SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_Renderer *renderer = SDL_CreateRenderer
							(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); 
	SDL_RenderSetScale(renderer, SCALE, SCALE); // scale SDL renderer
	const Uint8 *state = SDL_GetKeyboardState(NULL); // SDL scankey pointer
    SDL_Event event;

	// run emulator
    while(running)
    {
		// process SDL quit button and escape key
        while(SDL_PollEvent(&event)) 
			{
			// check for window resize
			if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED)
			{
				// scale the emulator within bounds of screen
				if(event.window.data1/64 < event.window.data2/32)
					SDL_RenderSetScale(renderer, event.window.data1/64, event.window.data1/64); 
				else SDL_RenderSetScale(renderer, event.window.data2/32, event.window.data2/32);
			}
			
			// check keyboard input
			else if(event.type == SDL_KEYDOWN) 
			{
				if(state[SDL_SCANCODE_ESCAPE]) running = false; // quit
		        else if(state[SDL_SCANCODE_P]) paused ^= true; // pause
		        else if(state[SDL_SCANCODE_O]) debug ^= true; // debug prints
			}			
			
			// close app
			else if(event.type == SDL_QUIT) running = false;
		}

    	
    	// dont emulate while paused
        if(paused) continue; 
        
        // process keyboard input
        for(int k = 0; k < 16; k++) key[k] = state[keyconvert[k]];
        
        // run 8 cpu cycles
        for(int i = 0; i < 8; i++) chip8_cycle(debug);
        
        // update timer
    	chip8_timerupdate();
    	
		// render frame at vsync
 		render_frame(renderer);
	}

	// release SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

