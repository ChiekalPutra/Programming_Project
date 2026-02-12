#include <SDL2/SDL_events.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <bits/posix1_lim.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <string.h>
#include <wchar.h>
#include "chip8.h"

void INIT_CHIP();
void DRAW();
void SDL_CLEAN();
void EMULATE();
uint32_t LOAD_ROM(char* file);

// Initialize SDL
SDL_Renderer * RENDERER;
SDL_Window   * WINDOW;
SDL_Texture  * SCREEN;

// Cleanup SDL
void SDL_CLEAN(){
    SDL_DestroyTexture(SCREEN);
    SDL_DestroyRenderer(RENDERER);
    SDL_DestroyWindow(WINDOW);
    SDL_Quit();
}

int main(int argc, char ** argv){
    uint32_t QUIT = 0;

    if(argc < 2){
        printf("USAGE: chip8 <ROM> \n");
        return 0;
    }

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0){
		fprintf(stderr, "SDL failed to initialise: %s\n", SDL_GetError());
		return 0;
	}
    SDL_Event event;
    WINDOW = SDL_CreateWindow(("CHIP-8:  %s",argv[1]),SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,640,320,0);
	RENDERER = SDL_CreateRenderer(WINDOW,-1,SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	SDL_RenderSetLogicalSize(RENDERER, 64, 32);
	SDL_SetRenderDrawColor(RENDERER, 0, 0, 0, 255);
	SDL_RenderClear(RENDERER);

    SCREEN = SDL_CreateTexture(RENDERER, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,64,32);

    INIT_CHIP();
    if(LOAD_ROM(argv[1]) == 0){
        SDL_CLEAN();
        return 0;
    }

    int32_t speed=5;

    while(!QUIT)
	{
		printf("Speed: %d \n",speed);
		while(SDL_PollEvent(&event))
		{
				switch(event.type)
				{
					case SDL_QUIT:
					QUIT = 1;
					break;

					case SDL_KEYDOWN:

					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:QUIT = 1;break;
						case SDLK_F1:
							INIT_CHIP();
							if (LOAD_ROM(argv[1]) == 0)
							{
								SDL_CLEAN();
								return 0;
							}
						break;
                    }
                }
        }
        if (speed < 0){
            speed = 0;
        }else{
            SDL_Delay(speed);
        }

        if(DELAY_TIMER > 0) --DELAY_TIMER;

        EMULATE();
        DRAW();
    }   

    SDL_CLEAN();
    return 0;
}

// Initialize the CPU
void INIT_CHIP(){
    DELAY_TIMER = 0;
    SOUND_TIMER = 0;
    OPCODE      = 0;
    PC          = 0x200;
    I           = 0;
    SP          = 0;

    memset(LEVEL_STACK, 0, 16);
    memset(MEMORY, 0, 4096);
    memset(V, 0, 16);
    memset(DISPLAY, 0, 2048);
    memset(KEYPAD, 0, 16);
}

// Loard the ROM file to MEMORY
uint32_t LOAD_ROM(char *file){
    FILE * fp = fopen(file, "rb");

    // if there is no rom loaded throw an error
    if(fp == NULL){
        fprintf(stderr, "ERROR CANT OPEN FILE\n");
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    fread(MEMORY + 0x200, sizeof(uint16_t), size, fp);
    return 1;
}

// Draw function
void DRAW()
{
	uint32_t pixels[64 * 32];
	unsigned int x, y;
		
	if (DRAWFLAG)
	{
		memset(pixels, 0, (64 * 32) * 4);
		for(x=0;x<64;x++)
		{
			for(y=0;y<32;y++)
			{
				if (DISPLAY[(x) + ((y) * 64)] == 1)
				{
					pixels[(x) + ((y) * 64)] = UINT32_MAX;
				}
			}
		}
		
		SDL_UpdateTexture(SCREEN, NULL, pixels, 64 * sizeof(uint32_t));
	
		SDL_Rect position;
		position.x = 0;
		position.y = 0;
		// If you change SDL_RenderSetLogicalSize, change this accordingly.
		position.w = 64;
		position.h = 32;
		SDL_RenderCopy(RENDERER, SCREEN, NULL, &position);
		SDL_RenderPresent(RENDERER);
	}
	DRAWFLAG = false;
}

// Emulate Cycle
void EMULATE(){
    uint8_t X, Y, nn, n;
    uint16_t nnn;
    uint32_t i;

    // FETCH
    OPCODE = MEMORY[PC] << 8 | MEMORY[PC + 1];
    PC += 2;
    X = (OPCODE & 0x0F00) >> 8;
    Y = (OPCODE & 0x00F0) >> 4;
    nnn = (OPCODE & 0x0FFF);
    nn = (OPCODE & 0x00FF);
    n = (OPCODE & 0x000F);

    // Print some info to TERMINAL
    printf("OPCODE: %x \n", OPCODE);
    printf("PC: %x\n", PC);
    printf("I: %x\n", I);

    switch (OPCODE & 0xF000) {
        
        case 0x000:

        switch (OPCODE & 0x00FF) {
            // 00E0 -  	Clears the screen
            case 0x00E0:
                memset(DISPLAY, 0, 2048);
                DRAWFLAG = true;
            break;

            // 00EE -  	Returns from a subroutine
            case 0x00EE:
                --SP;
                PC = LEVEL_STACK[SP];
            break;

            default: printf("OPCODE ERROR -> %x", OPCODE);
        }break;

        // 1nnn - Jumps to address NNN
        case 0x1000:
            PC = nnn;
        break;

        // 2nnn - Calls subroutine at NNN
        case 0x2000:
            LEVEL_STACK[SP] = PC;
            ++SP;
            PC = nnn;
        break;

        // 3xkk - Skips the next instruction if VX equals nn 
        case 0x3000:
            if (V[X] == nn) PC += 2;
        break;

        // 4xkk - Skips the next instruction if VX does not equal nn
        case 0x4000:
            if (V[X] != nn) PC += 2;
        break;

        // 5xy0 - Skips the next instruction if VX equals VY
        case 0x5000:
            if (V[X] == V[Y]) PC += 2;
        break;

        // 6xkk - Sets VX to nn
        case 0x6000:
            V[X] = nn;
        break;

        // 7xkk - add VX to nn
        case 0x7000:
            V[(X)] += nn;
        break;

        // 8xyn
        case 0x8000:
        switch (n) {
            
            // 8xy0 - Sets VX to the value of VY
            case 0x0000: 
                V[X] = V[Y];
            break;    
            
            // 8xy1 - Sets VX to VX or VY.
            case 0x0001:
                V[X] |= V[Y];
            break;

            // 8xy2 - Sets VX to VX and VY
            case 0x0002:
                V[X] &= V[Y];
            break;

            // 8xy3 - Sets VX to VX xor VY
            case 0x0003:
                V[X] ^= V[Y];
            break;

            // 8xy4 - Adds VY to VX. VF is set to 1 when there's an overflow, and to 0 when there is not
            case 0x0004:{
                int i;
                i = (int)(V[X]) + (int)(V[Y]);

                if (i > 255){
                    V[0xF] = 1;
                }else{
                    V[0xF] = 0;
                }

                V[X] = i & 0xFF;
            }
            break;

            // 8xy5 - VY is subtracted from VX. VF is set to 0 when there's an underflow, and 1 when there is not
            case 0x0005:
                if (V[X]> V[Y] ) V[0xF] = 1;
                else V[0xF] = 0;
				
                V[X] -= V[Y];
			break; 

            // 8xy6 - Shifts VX to the right by 1
            case 0x0006:
                V[0xF] = V[X] &1;
				V[X] >>= 1;
			break;

            // 8xy7 - Sets VX to VY minus VX. VF is set to 0 when there's an underflow, and 1 when there is not.
            case 0x0007:
                if(V[Y] > V[X]) V[0xF] = 1;
                else V[0xF] = 0;
				
                V[X] = V[Y] - V[X];
			break;

            // 8xye - Shifts VX to the left by 1, then sets VF to 1 if the most significant bit of VX prior to that shift was set, or to 0 if it was unset
            case 0x000E:
				V[0xF] = V[X] >> 7;
				V[X] <<= 1;
			break; 	

				default: printf("error -> %x\n",OPCODE );
        }
        break; 

        // Annn - Sets I to the address NNN
        case 0xA000:
            I = nnn;
        break;

        //Dxyn - Draws a sprite at coordinate (VX, VY)
		case 0xD000:;
		uint16_t x = V[X];
		uint16_t y = V[Y];
		uint16_t height = n;
		uint8_t pixel;

		V[0xF] = 0;
		for (int yline = 0; yline < height; yline++) {
			pixel = MEMORY[I + yline];
			for(int xline = 0; xline < 8; xline++) {
				if((pixel & (0x80 >> xline)) != 0) {
					if(DISPLAY[(x + xline + ((y + yline) * 64))] == 1){
						V[0xF] = 1;                                   
					}
					DISPLAY[x + xline + ((y + yline) * 64)] ^= 1;
				}

			}

		}
		DRAWFLAG = true;
		break;

        // Fxnn
        case 0xF000:
            switch (nn) {
                // Fx07 -  Sets VX to the value of the delay timer
                case 0x0007:
                    V[X] = DELAY_TIMER;
                break;

                // Fx15 - Sets the delay timer to VX.
                case 0x0015:
                    DELAY_TIMER = V[X];
                break;

                // Fx18 - Sets the sound timer to VX
                case 0x0018:
                    SOUND_TIMER = V[X];
                break;

                // Fx1E - Adds VX to I. VF is not affected
                case 0x001E:
                    I = I + V[X];
                break;

                // Fx29 - Sets I to the location of the sprite for the character in VX(only consider the lowest nibble)
                case 0x0029:
                    I = V[X] * 5;
                break;

                // Fx33 - Stores the binary-coded decimal representation of VX, with the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2
                case 0x0033:{
                    int vX;
					vX = V[X];
					MEMORY[I]     = (vX - (vX % 100)) / 100;
					vX -= MEMORY[I] * 100;
					MEMORY[I + 1] = (vX - (vX % 10)) / 10;
					vX -= MEMORY[I+1] * 10;
					MEMORY[I + 2] = vX;
                }
                break;

                // Fx55 - Stores from V0 to VX (including VX) in memory, starting at address I
                case 0x0055:
                    for (uint8_t i = 0; i <= X; ++i){
					    MEMORY[I+ i] = V[i];	
				    }
                break;

                // Fx65 - Fills from V0 to VX (including VX) with values from memory, starting at address I.
                case 0x0065:
                    for (uint8_t i = 0; i <= X; ++i){
					    V[i] = MEMORY[I+ i];	
				    }
                break;
            }
        break;

    }
}