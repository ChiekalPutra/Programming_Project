#include <SDL2/SDL_events.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
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
    uint8_t X, Y, kk, n;
    uint16_t nnn;
    uint32_t i;

    // FETCH
    OPCODE = MEMORY[PC] << 8 | MEMORY[PC + 1];
    PC += 2;
    X = (OPCODE & 0x0F00) >> 8;
    Y = (OPCODE & 0x00F0) >> 4;
    nnn = (OPCODE & 0x0FFF);
    kk = (OPCODE & 0x00FF);
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

        // 1nnn -  	Jumps to address NNN
        case 0x1000:
            PC = nnn;
        break;

        // 6xkk - Sets VX to kk
        case 0x6000:
            V[X] = kk;
        break;

        // 7xkk - add VX to kk
        case 0x7000:
            V[(X)] += kk;
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
    }
}