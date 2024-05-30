#include <SDL/SDL.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "chip8.h"

#define DEBUG 0

// 64x32 pixels; 16x16 (256) pixels per pixel
const uint32_t PIXEL_SIZE = 16u;                  // 16u;
const uint16_t SCREEN_WIDTH = 1024u;              // 64 * 16
const uint16_t SCREEN_HEIGHT = 512u;              // 32 * 16
const uint16_t SCREEN_TICKS_PER_OP = 1000u / 60u; // 1000ms / OPS_PER_SECOND

//                        0xAARRGGBB
const uint32_t PIXEL_ON = 0xFF2051A9;  // darker cornflower blue
const uint32_t PIXEL_OFF = 0xFF6495ED; // cornflower blue

// individual pixels
static void setPixel(SDL_Surface *surface, uint16_t x, uint16_t y, uint8_t on)
{
    uint32_t startX = x;
    uint32_t startY = y;
    uint32_t *const targetPixel = (uint32_t *)(((uint8_t *)surface->pixels) + (startY * surface->pitch) + (startX * surface->format->BytesPerPixel));
    *targetPixel = on ? PIXEL_ON : PIXEL_OFF;
}

// big pixels
static void setPixelA(SDL_Surface *surface, uint16_t x, uint16_t y, uint8_t on)
{
    uint32_t startX = x * PIXEL_SIZE;
    uint32_t startY = y * PIXEL_SIZE;
    uint32_t *const targetPixel = (uint32_t *)(((uint8_t *)surface->pixels) + (startY * surface->pitch) + (startX * surface->format->BytesPerPixel));
    // *targetPixel = on ? PIXEL_ON : PIXEL_OFF;

    // for PIXEL_SIZE rows ...
    for (uint32_t Y = 0; Y < PIXEL_SIZE; ++Y)
    {
        // ... set PIXEL_SIZE bytes to the desired colour
        // memset(targetPixel + (surface->pitch * (Y - startY)), on ? PIXEL_ON : PIXEL_OFF, PIXEL_SIZE);

        for (uint32_t X = 0; X < PIXEL_SIZE; ++X)
        {
            targetPixel[X + (Y * SCREEN_WIDTH)] = on ? PIXEL_ON : PIXEL_OFF;
        }
    }
}

// draw the chip8 display buffer onto the surface
// each bit represents a screen pixel (on or off)
// from 0xF00 to 0xFFF, 32 rows of 64, total 2048 (0x800) pixels
//  in 32*(64/8)=128=0x100 bytes
static void renderScreen(SDL_Surface *surface, Chip8State *chip8State)
{
    uint8_t x;
    // 32 rows of screen
    for (uint8_t y = 0; y < 32; ++y)
    {
        // 64 columns of screen, stored in 64 bits across 8 bytes
        for (uint8_t xByte = 0; xByte < 8; ++xByte)
        {
            // y*8 because the screen is 8 bytes across
            uint8_t byteIdx = xByte + (y * 8);
            uint8_t byte = chip8State->screen[byteIdx];

            // for each bit of the byte
            for (uint8_t pixel = 0; pixel < 8; ++pixel)
            {
                // determine x coordinate of the pixel in the screen
                x = (xByte * 8) + pixel;
                // determine the status of the bit
                uint8_t bit = byte & (128 >> pixel);
                setPixelA(surface, x, y, bit > 0);
            }
        }
    }
}

static void interpretKeyPress(Chip8State *chip8State, SDL_Keycode key)
{
    uint8_t keyValue = 0x10;
    switch (key)
    {
    case SDLK_0:
    case SDLK_1:
    case SDLK_2:
    case SDLK_3:
    case SDLK_4:
    case SDLK_5:
    case SDLK_6:
    case SDLK_7:
    case SDLK_8:
    case SDLK_9:
        keyValue = key - 48;
        break;
    case SDLK_a:
    case SDLK_b:
    case SDLK_c:
    case SDLK_d:
    case SDLK_e:
    case SDLK_f:
        keyValue = key - 97;
        break;
    default:
        break;
    }

    if (keyValue > 0xF)
        return;

    chip8State->keys[keyValue] = 1;
}

static void clearKeys(Chip8State *chip8State)
{
    for (uint8_t k = 0; k < 16; ++k)
    {
        chip8State->keys[k] = 0;
    }
}

int main(int argc, char **argv)
{
    // check args
    if (argc != 2)
    {
        printf("ERROR: Must specify file to disassemble\n");
        return -1;
    }

    // read file
    FILE *file = fopen(argv[1], "rb");
    if (!file)
    {
        printf("ERROR: Couldn't open %s\n", argv[1]);
        return -2;
    }

    // get file size
    fseek(file, 0L, SEEK_END);
    int fsize = ftell(file);
    fseek(file, 0L, SEEK_SET);

    // init CHIP8
    Chip8State *chip8State = InitChip8();

    // CHIP-8 convention puts programs into RAM at 0x200
    // ROMs will be hardcoded to expect that

    // read the file into RAM at 0x200 and close it
    fread(chip8State->memory + 0x200, fsize, 1, file);
    fclose(file);

#if DEBUG
    // output register values
    printf("0:%02x 1:%02x 2:%02x 3:%02x 4:%02x 5:%02x 6:%02x 7:%02x 8:%02x 9:%02x A:%02x B:%02x C:%02x D:%02x E:%02x F:%02x I:%03x PC:%03x instr:%04x\n",
           chip8State->V[0x0],
           chip8State->V[0x1],
           chip8State->V[0x2],
           chip8State->V[0x3],
           chip8State->V[0x4],
           chip8State->V[0x5],
           chip8State->V[0x6],
           chip8State->V[0x7],
           chip8State->V[0x8],
           chip8State->V[0x9],
           chip8State->V[0xA],
           chip8State->V[0xB],
           chip8State->V[0xC],
           chip8State->V[0xD],
           chip8State->V[0xE],
           chip8State->V[0xF],
           chip8State->I,
           chip8State->PC,
           (chip8State->memory[chip8State->PC] << 8) | chip8State->memory[chip8State->PC + 1]);
#endif

    // initialise sdl and video subsystem
    SDL_Window *window = NULL;
    SDL_Surface *surface = NULL;
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL initialisation failed: %s\n", SDL_GetError());
        return -3;
    }

    // create a window
    window = SDL_CreateWindow("Chip 8 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE); // | SDL_WINDOW_BORDERLESS
    if (window == NULL)
    {
        printf("SDL failed to create window: %s\n", SDL_GetError());
        return -4;
    }

    // get the window surface
    surface = SDL_GetWindowSurface(window);
    // fill it with a colour
    // SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0x64, 0x95, 0xED));
    // update the surface
    // SDL_UpdateWindowSurface(window);

    renderScreen(surface, chip8State);
    SDL_UpdateWindowSurface(window);

    // force window to stay open until closed
    SDL_Event e;
    int quit = 0;
    int advanceFrame = 0;
    uint32_t prevTime = SDL_GetTicks();
    // loop frames until we want to quit
    while (!quit)
    {
        // poll for an event
        while (SDL_PollEvent(&e))
        {
            // clear the key states before processing
            clearKeys(chip8State);

            if (e.type == SDL_QUIT)
            {
                quit = 1;
            }
            else if (e.type == SDL_KEYDOWN)
            {
                switch (e.key.keysym.sym)
                {
                case SDLK_SPACE:
                    advanceFrame = 1;
                    break;
                case SDLK_ESCAPE:
                    quit = 1;
                    break;
                default:
                    interpretKeyPress(chip8State, e.key.keysym.sym);
                    break;
                }
            }
        }

        if (!advanceFrame)
        {
            // run emulator; execute program
            EmulateChip8(chip8State);

#if DEBUG
            // output register values
            printf("0:%02x 1:%02x 2:%02x 3:%02x 4:%02x 5:%02x 6:%02x 7:%02x 8:%02x 9:%02x A:%02x B:%02x C:%02x D:%02x E:%02x F:%02x I:%03x PC:%03x instr:%04x\n",
                   chip8State->V[0x0],
                   chip8State->V[0x1],
                   chip8State->V[0x2],
                   chip8State->V[0x3],
                   chip8State->V[0x4],
                   chip8State->V[0x5],
                   chip8State->V[0x6],
                   chip8State->V[0x7],
                   chip8State->V[0x8],
                   chip8State->V[0x9],
                   chip8State->V[0xA],
                   chip8State->V[0xB],
                   chip8State->V[0xC],
                   chip8State->V[0xD],
                   chip8State->V[0xE],
                   chip8State->V[0xF],
                   chip8State->I,
                   chip8State->PC,
                   (chip8State->memory[chip8State->PC] << 8) | chip8State->memory[chip8State->PC + 1]);
#endif

            renderScreen(surface, chip8State);
            SDL_UpdateWindowSurface(window);

            advanceFrame = 0;
        }

        // time at end of frame
        uint32_t timeDiff = (SDL_GetTicks() - prevTime);
        if (timeDiff < SCREEN_TICKS_PER_OP)
        {
            SDL_Delay(SCREEN_TICKS_PER_OP - timeDiff);
        }
        prevTime = SDL_GetTicks();
    }

    // destroy the window and quit the subsystems
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
