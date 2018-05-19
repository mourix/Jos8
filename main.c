/******************************************************************************
main.c
Jos8: a work in progress CHIP-8 emulator using C and SDL2

(c) 2018 Jos van Mourik
******************************************************************************/

// includes
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "chip8.h"
#include "SDL2/SDL.h"


// global variables and settings
float scale = 10; // scaling of 64x32 CHIP-8 screen
bool running, paused = false; // running/pause state
bool debug = false; // debug state
bool screen_wrap = false; // enable DXYN screen wrapping
bool cowgod = true; // enable Cowgod's 8XY6/8XYE+FX55/FX65 syntax
unsigned char xoffset, yoffset = 0; // screen resize offset

// SDL snancode to CHIP-8 keycode conversion
const int keyconvert[16] = 
{
	SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, 
	SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A,
	SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C, 
	SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V
};

// render a full frame
void render_frame(SDL_Renderer *renderer, unsigned char xo, unsigned char yo)
{
	for(int y = 0; y < 32; y++)
	{			
		for(int x = 0; x < 64; x++)
		{
			if(screen[x][y]) // white pixel
			{
				SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
				SDL_RenderDrawPoint(renderer, x+xo, y+yo);
			}
			else // black pixel
			{
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
				SDL_RenderDrawPoint(renderer, x+xo, y+yo);
			}
		}					
	}
	SDL_RenderPresent(renderer); // update screen
}

// update SDL window title
void update_SDL_title(SDL_Window *window)
{
	char buf[50] = {0};
	strcpy(buf, "Jos8");
	if(paused) strcat (buf, " (paused)");
	if(debug) strcat (buf, " - debug-output");
	if(screen_wrap) strcat (buf, " - screen-wrapping");
	if(cowgod) strcat (buf, " - Cowgod-syntax");
	SDL_SetWindowTitle(window, buf); 
}

// update SDL window scale and offset
void update_SDL_size(SDL_Event event, SDL_Renderer *renderer)
{
	// scale the emulator within bounds of screen
	if(event.window.data1/64 < event.window.data2/32) scale = event.window.data1/64;
	else scale = event.window.data2/32;
	
	// center image
	xoffset = (int)(event.window.data1 - (scale*64))/2/scale;
	yoffset = (int)(event.window.data2 - (scale*32))/2/scale;
	
	// render
	SDL_RenderSetScale(renderer, scale, scale);
	render_frame(renderer, xoffset, yoffset);
}

// main emulator loop
int main(int argc, char *argv[]) 
{
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
						 SDL_WINDOWPOS_UNDEFINED, 64*scale, 32*scale,
						 SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	update_SDL_title(window);
	SDL_Renderer *renderer = SDL_CreateRenderer
							(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); 
	SDL_RenderSetScale(renderer, scale, scale); // scale SDL renderer
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
 				update_SDL_size(event, renderer);
			
			// check user input
			else if(event.type == SDL_KEYDOWN) 
			{
				if(state[SDL_SCANCODE_ESCAPE]) running = false; // quit
		        else if(state[SDL_SCANCODE_P]) paused ^= true; // pause
		        else if(state[SDL_SCANCODE_F5]) chip8_init(); // reset emulation
				else if(state[SDL_SCANCODE_F6]) screen_wrap ^= true; // screen wrapping
		        else if(state[SDL_SCANCODE_F7]) cowgod ^= true; // Cowgod syntax
		        else if(state[SDL_SCANCODE_F8]) debug ^= true; // debug prints
		        update_SDL_title(window);
			}			
			
			// close app
			else if(event.type == SDL_QUIT) running = false;
		}

    	// dont emulate while paused
        if(paused) continue; 
        
        // process keyboard input
        for(int k = 0; k < 16; k++) key[k] = state[keyconvert[k]];
        
        // run 8 cpu cycles
        for(int i = 0; i < 8; i++) chip8_cycle(debug, screen_wrap, cowgod);
        
        // update timer
    	chip8_timerupdate();
    	
		// render frame at vsync
 		render_frame(renderer, xoffset, yoffset);
	}

	// release SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

