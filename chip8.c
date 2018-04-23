/******************************************************************************
chip8.c
CHIP-8 cpu interpreter.
(c) 2018 Jos van Mourik

Based on multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter
******************************************************************************/

// includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chip8.h"

// defines
#define DEBUG
unsigned char cowgod = 1; // enable Cowgod's 8XY6/8XYE syntax

// global variables
unsigned char memory[4096]; // program memory
unsigned short opcode; // current opcode
unsigned short pc; // program counter
unsigned char V[16]; // data register
unsigned short I; // index register
unsigned short stack[16]; // stack 
unsigned short sp; // stack pointer
unsigned char delay_timer; // delay timer
unsigned char sound_timer; // sound timer
unsigned char screen[64][32];// screen
unsigned char key[16]; // input
unsigned char drawflag; // flag for screen update
unsigned char timercount; // counter for timer clock devide

// initialize all memory and registers
void chip8_init(void)
{
	// clear screen
	memset(screen, 0, sizeof(screen[0][0])*64*32);
	
	// clear memory, registers and stack
	pc = 0x200;
	memset(V, 0, sizeof(V));
	I = 0;
	memset(stack, 0, sizeof(stack));
	sp = 0;
	opcode = 0;
	memset(memory, 0, sizeof(memory));

	// load fontset
	for(int i = 0; i < 80; i++)
	{
		memory[i+0x50] = chip8_fontset[i];
	}
	
	// reset timers 
	delay_timer = 0;
	sound_timer = 0;
	timercount = 0; 
	
	printf("CHIP-8 initialized succesfully\n");
}

// load rom file into memory
int chip8_load(char* rom)
{
	// open file
	FILE * fp = fopen(rom, "rb");
	
	// check if file exists
	if(fp == NULL)
	{
		printf("ERROR opening %s\n", rom);
		return 1;
	}
	
	// get file size
	fseek(fp, 0, SEEK_END);
	size_t len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	// read rom file into buffer
	unsigned char *buffer = malloc(len);
	fread(buffer, len, 1, fp);
	
	// check for memory overflow
	if (len <= (4096-0x200)) printf("Loaded %s\n", rom);
	
	// read buffer into memory
	for(int i = 0; i < len; i++) memory[i+0x200] = buffer[i];
	
	// close file and free buffer
	fclose(fp);
	free(buffer);
	return 0;
}

// emulate a single cpu cycle
int chip8_cycle(void)
{
	// flag for screen update
	drawflag = 0;
	
	// fetch opcode
	opcode = memory[pc] << 8 | memory[pc + 1];
	
	// decode opcode
	switch(opcode & 0xF000)
	{
		case 0x0000:
			switch(opcode & 0x000F)
			{
				// 00E0 	Display 	disp_clear() 	Clears the screen.	
				case 0x0000:
					memset(screen, 0, sizeof(screen[0][0])*64*32);
					pc += 2;
					drawflag = 1; // update screen
					#ifdef DEBUG
					printf ("Opcode 0x%X: 00E0 disp_clear\n", opcode);
					#endif
					break;
				
				// 00EE 	Flow 	return; 	Returns from a subroutine.
				case 0x000E:
					sp--;
					pc = stack[sp];
					#ifdef DEBUG
					printf ("Opcode 0x%X: 00EE return\n", opcode);
					#endif
					break;
					
				default:
					printf ("Unknown opcode: 0x%X\n", opcode);
			}
			break;
		
		// 1NNN 	Flow 	goto NNN; 	Jumps to address NNN.
		case 0x1000:
			pc = opcode & 0x0FFF;
			#ifdef DEBUG
			printf ("Opcode 0x%X: 1NNN goto NNN\n", opcode);
			#endif
			break;
			
		// 2NNN 	Flow 	*(0xNNN)() 	Calls subroutine at NNN.	
		case 0x2000:
			stack[sp] = pc + 2;
			sp++;
			pc = opcode & 0x0FFF;
			#ifdef DEBUG
			printf ("Opcode 0x%X: 2NNN Call subroutine NNN\n", opcode);
			#endif
			break;
			
		// 3XNN 	Cond 	if(Vx==NN) 	Skips the next instruction if VX equals NN. 	
		case 0x3000:
			if(V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) pc += 4;
			else pc += 2;
			#ifdef DEBUG
			printf ("Opcode 0x%X: 3XNN skip if(Vx==NN)\n", opcode);
			#endif
			break;
				
		// 4XNN 	Cond 	if(Vx!=NN) 	Skips the next instruction if VX doesn't equal NN. 	
		case 0x4000:
			if(V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) pc += 4;
			else pc += 2;		
			#ifdef DEBUG
			printf ("Opcode 0x%X: 4XNN skip if(Vx!=NN)\n", opcode);
			#endif
			break;
			
		// 5XY0 	Cond 	if(Vx==Vy) 	Skips the next instruction if VX equals VY. 
		case 0x5000:
			if(V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]) pc += 4;
			else pc += 2;
			#ifdef DEBUG
			printf ("Opcode 0x%X: 5XY0 skip if(Vx==Vy)\n", opcode);
			#endif
			break;
			
		// 6XNN 	Const 	Vx = NN 	Sets VX to NN.	
		case 0x6000:
			V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
			pc += 2;
			#ifdef DEBUG
			printf ("Opcode 0x%X: 6XNN Vx = NN \n", opcode);
			#endif
			break;
			
		// 7XNN 	Const 	Vx += NN 	Adds NN to VX. (Carry flag is not changed)
		case 0x7000:
			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] + (opcode & 0x00FF);
			pc += 2;
			#ifdef DEBUG
			printf ("Opcode 0x%X: 7XNN Vx += NN\n", opcode);
			#endif
			break;
			
		case 0x8000:
			switch(opcode & 0x000F)
			{
				// 8XY0 	Assign 	Vx=Vy 	Sets VX to the value of VY.
				case 0x0000:
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
					pc += 2;
					#ifdef DEBUG
					printf ("Opcode 0x%X: 8XY0 Vx=Vy\n", opcode);
					#endif
					break;
					
				// 8XY1 	BitOp 	Vx=Vx|Vy 	Sets VX to VX or VY. (Bitwise OR operation)
				case 0x0001:
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] | V[(opcode & 0x00F0) >> 4];
					pc += 2;
					#ifdef DEBUG
					printf ("Opcode 0x%X: 8XY1 Vx|Vy\n", opcode);
					#endif
					break;
					
				// 8XY2 	BitOp 	Vx=Vx&Vy 	Sets VX to VX and VY. (Bitwise AND operation)
				case 0x0002:
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] & V[(opcode & 0x00F0) >> 4];
					pc += 2;
					#ifdef DEBUG
					printf ("Opcode 0x%X: 8XY2 Vx=Vx&Vy\n", opcode);
					#endif
					break;
					
				// 8XY3 	BitOp 	Vx=Vx^Vy 	Sets VX to VX xor VY.
				case 0x0003:
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] ^ V[(opcode & 0x00F0) >> 4];
					pc += 2;
					#ifdef DEBUG
					printf ("Opcode 0x%X: 8XY3 Vx=Vx^Vy\n", opcode);
					#endif
					break;

				// 8XY4 	Math 	Vx += Vy 	Adds VY to VX. 
				// VF is set to 1 when there's a carry, and to 0 when there isn't.
				case 0x0004:
					if((V[(opcode & 0x0F00) >> 8] + V[(opcode & 0x00F0) >> 4]) > 255) V[0xF] = 1;
					else V[0xF] = 0;
					V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
					pc += 2;
					#ifdef DEBUG
					printf ("Opcode 0x%X: 8XY4 Vx += Vy\n", opcode);
					#endif
					break;
					
				// 8XY5 	Math 	Vx -= Vy 	VY is subtracted from VX. 
				// VF is set to 0 when there's a borrow, and 1 when there isn't.
				case 0x0005:
					if(V[(opcode & 0x0F00) >> 8] < V[(opcode & 0x00F0) >> 4]) V[0xF] = 0;
					else V[0xF] = 1;
					V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
					pc += 2;
					#ifdef DEBUG
					printf ("Opcode 0x%X: 8XY5 Vx -= Vy\n", opcode);
					#endif
					break;
					
				// 8XY6 	BitOp 	Vx=Vy=Vy>>1 	Shifts VY right by one and copies the result to VX. 
				// VF is set to the value of the least significant bit of VY before the shift.
				case 0x0006:
					if(cowgod)
					{
						V[0xF] = V[(opcode & 0x0F00) >> 8] & 1;
						V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] >> 1;
					}
					else
					{
						V[0xF] = V[(opcode & 0x00F0) >> 4] & 1;
						V[(opcode & 0x00F0) >> 4] = V[(opcode & 0x00F0) >> 4] >> 1;
						V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
					}
					pc += 2;
					#ifdef DEBUG
					printf ("Opcode 0x%X: 8XY6 Vx=Vy=Vy>>1\n", opcode);
					#endif
					break;
					
				// 8XY7 	Math 	Vx=Vy-Vx 	Sets VX to VY minus VX. 
				// VF is set to 0 when there's a borrow, and 1 when there isn't.
				case 0x0007:
					if(V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4]) V[0xF] = 0;
					else V[0xF] = 1;
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
					pc += 2;
					#ifdef DEBUG
					printf ("Opcode 0x%X: 8XY7 Vx=Vy-Vx\n", opcode);
					#endif
					break;
					
				// 8XYE 	BitOp 	Vx=Vy=Vy<<1 	Shifts VY left by one and copies the result to VX. 
				// VF is set to the value of the most significant bit of VY before the shift.
				case 0x000E:
					if(cowgod)
					{
						V[0xF]  = (V[(opcode & 0x0F00) >> 8] & 128) >> 7;
						V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] << 1;
					}
					else
					{
						V[0xF] = (V[(opcode & 0x00F0) >> 4] & 128) >> 7;
						V[(opcode & 0x00F0) >> 4] = V[(opcode & 0x00F0) >> 4] << 1;	
						V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
					}
					pc += 2;
					#ifdef DEBUG
					printf ("Opcode 0x%X: 8XYE Vx=Vy=Vy<<1\n", opcode);
					#endif
					break;
	
				default:
					printf ("Unknown opcode: 0x%X\n", opcode);
			}
			break;
			
		// 9XY0 	Cond 	if(Vx!=Vy) 	Skips the next instruction if VX doesn't equal VY. 
		// (Usually the next instruction is a jump to skip a code block)
		case 0x9000:
			if(V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4]) pc += 4;
			else pc += 2;
			#ifdef DEBUG
			printf ("Opcode 0x%X: 9XY0 skip if(Vx!=Vy)\n", opcode);
			#endif
			break;
			
		// 	ANNN 	MEM 	I = NNN 	Sets I to the address NNN.
		case 0xA000:
			I = opcode & 0x0FFF;
			pc += 2;
			#ifdef DEBUG
			printf ("Opcode 0x%X: ANNN I = NNN\n", opcode);
			#endif
			break;
			
		// BNNN 	Flow 	PC=V0+NNN 	Jumps to the address NNN plus V0.
		case 0xB000:
			pc = (opcode & 0x0FFF) + V[0];
			#ifdef DEBUG
			printf ("Opcode 0x%X: BNNN PC=V0+NNN\n", opcode);
			#endif
			break;
			
		// 	CXNN 	Rand 	Vx=rand()&NN 	
		// Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN.
		case 0xC000:
			V[(opcode & 0x0F00) >> 8] = (rand() % 256) & (opcode & 0x00FF);
			pc += 2;
			#ifdef DEBUG
			printf ("Opcode 0x%X: CXNN Vx=rand()&NN\n", opcode);
			#endif
			break;	
			
		// 	DXYN 	Disp 	draw(Vx,Vy,N) 	
		// Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels.
		case 0xD000:;
			unsigned char xs = V[(opcode & 0x0F00) >> 8];
			unsigned char ys = V[(opcode & 0x00F0) >> 4];
			unsigned char n = opcode & 0x000F;
			unsigned char spritebyte;		
			V[0xF] = 0;
			
			// sprite draw routine
			for (int y = 0; y < n; y++)
			{
				spritebyte = memory[I+y];
				for(int x = 0; x < 8; x++)
				{
					if(((x+xs) < 64) && ((y+ys) < 32)) // out of screen detect
					{
						if(screen[x+xs][y+ys] && (spritebyte & (0x80 >> x))) V[0xF] = 1; // collision detect
						screen[x+xs][y+ys] ^= (spritebyte & (0x80 >> x)); // draw pixel
					}
				}
			}
			
			pc += 2;
			drawflag = 1; // update screen
			#ifdef DEBUG
			printf ("Opcode 0x%X: DXYN draw(Vx,Vy,N)\n", opcode);
			#endif
			break;
			
		case 0xE000:
			switch(opcode & 0x000F)
			{
				// EX9E 	KeyOp 	if(key()==Vx) 	Skips the next instruction if the key stored in VX is pressed. 
				case 0x000E:
					if(key[V[(opcode & 0x0F00) >> 8]]) pc += 4;
					else pc += 2;					
					#ifdef DEBUG
					printf ("Opcode 0x%X: EX9E if(key()==Vx)\n", opcode);
					#endif
					break;
				
				// EXA1 	KeyOp 	if(key()!=Vx) 	Skips the next instruction if the key stored in VX isn't pressed. 
				case 0x0001:
					if(!key[V[(opcode & 0x0F00) >> 8]]) pc += 4;
					else pc += 2;
					#ifdef DEBUG
					printf ("Opcode 0x%X: EXA1 if(key()!=Vx)\n", opcode);
					#endif
					break;
				
				default:
				printf ("Unknown opcode: 0x%X\n", opcode);
			}
			break;
			
		case 0xF000:
			switch(opcode & 0x00FF)
			{
				// FX07 	Timer 	Vx = get_delay() 	Sets VX to the value of the delay timer.
				case 0x0007:
					V[(opcode & 0x0F00) >> 8] = delay_timer;
					pc += 2;
					#ifdef DEBUG
					printf ("Opcode 0x%X: FX07 Vx = get_delay()\n", opcode);
					#endif
					break;
					
				// FX0A 	KeyOp 	Vx = get_key() 	A key press is awaited, and then stored in VX. 
				// (Blocking Operation. All instruction halted until next key event)
				case 0x000A:
					for(int i = 0; i < 15; i++)
					{
						if(key[i] == 1)
						{
							V[(opcode & 0x0F00) >> 8] = i;
							pc += 2;
						}
					}
					#ifdef DEBUG
					printf ("Opcode 0x%X: FX0A\n", opcode);
					#endif
					break;	
								
				// FX15 	Timer 	delay_timer(Vx) 	Sets the delay timer to VX.
				case 0x0015:
					delay_timer = V[(opcode & 0x0F00) >> 8];
					pc += 2;
					#ifdef DEBUG
					printf ("Opcode 0x%X: FX15 delay_timer(Vx)\n", opcode);
					#endif
					break;		
							
				// FX18 	Sound 	sound_timer(Vx) 	Sets the sound timer to VX.
				case 0x0018:
					sound_timer = V[(opcode & 0x0F00) >> 8];
					pc += 2;
					#ifdef DEBUG
					printf ("Opcode 0x%X: FX18 sound_timer(Vx)\n", opcode);
					#endif
					break;
					
				// FX1E 	MEM 	I +=Vx 	Adds VX to I.[3]
				case 0x001E:
					I += V[(opcode & 0x0F00) >> 8];
					pc += 2;
					#ifdef DEBUG
					printf ("Opcode 0x%X: FX1E I +=Vx\n", opcode);
					#endif
					break;	
								
				// FX29 	MEM 	I=sprite_addr[Vx] 	Sets I to the location of the sprite for the character in VX. 
				case 0x0029:
					I = 0x50 + (V[(opcode & 0x0F00) >> 8] * 5);
					pc += 2;
					#ifdef DEBUG
					printf ("Opcode 0x%X: FX29 I=sprite_addr[Vx] \n", opcode);
					#endif
					break;	
					
				// FX33 	BCD 	set_BCD(Vx); Stores the binary-coded decimal representation of VX
				case 0x0033:
					memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
					memory[I+1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
					memory[I+2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
					pc += 2;
					#ifdef DEBUG
					printf ("Opcode 0x%X: FX33 set_BCD(Vx);\n", opcode);
					#endif
					break;
					
				// FX55 	MEM 	reg_dump(Vx,&I) 	Stores V0 to VX (including VX) in memory starting at address I. 
				// I is increased by 1 for each value written.
				case 0x0055:
					for(int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
					{
						memory[I] = V[i];
						I++;
					}
					pc += 2;
					#ifdef DEBUG
					printf ("Opcode 0x%X: FX55 reg_dump(Vx,&I)\n", opcode);
					#endif
					break;	
								
				// FX65 	MEM 	reg_load(Vx,&I) 	Fills V0 to VX (including VX) with values from memory starting at address I. 
				// I is increased by 1 for each value written.
				case 0x0065:
					for(int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
					{
						V[i] = memory[I];
						I++;
					}						
					pc += 2;
					#ifdef DEBUG
					printf ("Opcode 0x%X: FX65 reg_load(Vx,&I)\n", opcode);
					#endif
					break;	
								
				default:
				printf ("Unknown opcode: 0x%X\n", opcode);
			}
			break;			

		default:
			printf ("Unknown opcode: 0x%X\n", opcode);
	}

	// update delay timer	
	if (delay_timer && (timercount == 7)) delay_timer--;
	
	// update sound timer
	if (sound_timer && (timercount == 7))
	{
		if (sound_timer == 1) printf("Beep...\n\a"); // plays OS beep
		sound_timer--;
	}
	
	// timer update for 60Hz
	if(timercount == 7) timercount = 0;
	else timercount++;
	
	// screen update status
	return drawflag;
}
