#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// constants
const uint16_t MEMORY_CAPACITY = 4096u;
const uint8_t DISPLAY_WIDTH = 64u;
const uint8_t DISPLAY_HEIGHT = 32u;
const uint16_t PROGRAM_BUFFER = 0x200u;
const uint16_t DISPLAY_BUFFER = 0xF00u;
const uint16_t STACK_BUFFER = 0xEA0u;

typedef struct Chip8State
{
    // RAM and Registers
    uint8_t V[0x10];     // 16 8-bit registers
    uint8_t *memory;     // RAM
    uint8_t *screen;     // display buffer - memory[0xF00]
    uint8_t keys[0x10];  // 16 key states (1 down, 0 up)
    uint16_t I;          // memory address register
    uint16_t SP;         // stack pointer
    uint16_t PC;         // program counter/index
    uint8_t delay;       // timer
    uint8_t sound;       // timer
    uint8_t awaitingKey; // flag showing whether we are waiting for input
} Chip8State;

/**
 * Sets first 80 (0x50) bytes of memory to the sprites for chars 0-F
 */
static void InsertFontIntoMemory(Chip8State *state)
{
    // 0
    state->memory[0x00] = 0b11110000;
    state->memory[0x01] = 0b10010000;
    state->memory[0x02] = 0b10010000;
    state->memory[0x03] = 0b10010000;
    state->memory[0x04] = 0b11110000;
    // 1
    state->memory[0x05] = 0b00100000;
    state->memory[0x06] = 0b01100000;
    state->memory[0x07] = 0b00100000;
    state->memory[0x08] = 0b00100000;
    state->memory[0x09] = 0b01110000;
    // 2
    state->memory[0x0A] = 0b11110000;
    state->memory[0x0B] = 0b00010000;
    state->memory[0x0C] = 0b11110000;
    state->memory[0x0D] = 0b10000000;
    state->memory[0x0E] = 0b11110000;
    // 3
    state->memory[0x0F] = 0b11110000;
    state->memory[0x10] = 0b00010000;
    state->memory[0x11] = 0b11110000;
    state->memory[0x12] = 0b00010000;
    state->memory[0x13] = 0b11110000;
    // 4
    state->memory[0x14] = 0b10010000;
    state->memory[0x15] = 0b10010000;
    state->memory[0x16] = 0b11110000;
    state->memory[0x17] = 0b00010000;
    state->memory[0x18] = 0b00010000;
    // 5
    state->memory[0x19] = 0b11110000;
    state->memory[0x1A] = 0b10000000;
    state->memory[0x1B] = 0b11110000;
    state->memory[0x1C] = 0b00010000;
    state->memory[0x1D] = 0b11110000;
    // 6
    state->memory[0x1E] = 0b11110000;
    state->memory[0x1F] = 0b10000000;
    state->memory[0x20] = 0b11110000;
    state->memory[0x21] = 0b10010000;
    state->memory[0x22] = 0b11110000;
    // 7
    state->memory[0x23] = 0b11110000;
    state->memory[0x24] = 0b00010000;
    state->memory[0x25] = 0b00100000;
    state->memory[0x26] = 0b01000000;
    state->memory[0x27] = 0b01000000;
    // 8
    state->memory[0x28] = 0b11110000;
    state->memory[0x29] = 0b10010000;
    state->memory[0x2A] = 0b11110000;
    state->memory[0x2B] = 0b10010000;
    state->memory[0x2C] = 0b11110000;
    // 9
    state->memory[0x2D] = 0b11110000;
    state->memory[0x2E] = 0b10010000;
    state->memory[0x2F] = 0b11110000;
    state->memory[0x30] = 0b00010000;
    state->memory[0x31] = 0b00010000;
    // a
    state->memory[0x32] = 0b11110000;
    state->memory[0x33] = 0b10010000;
    state->memory[0x34] = 0b11110000;
    state->memory[0x35] = 0b10010000;
    state->memory[0x36] = 0b10010000;
    // b
    state->memory[0x37] = 0b11100000;
    state->memory[0x38] = 0b10010000;
    state->memory[0x39] = 0b11100000;
    state->memory[0x3A] = 0b10010000;
    state->memory[0x3B] = 0b11100000;
    // c
    state->memory[0x3C] = 0b11110000;
    state->memory[0x3D] = 0b10000000;
    state->memory[0x3E] = 0b10000000;
    state->memory[0x3F] = 0b10000000;
    state->memory[0x40] = 0b11110000;
    // d
    state->memory[0x41] = 0b11100000;
    state->memory[0x42] = 0b10010000;
    state->memory[0x43] = 0b10010000;
    state->memory[0x44] = 0b10010000;
    state->memory[0x45] = 0b11100000;
    // e
    state->memory[0x46] = 0b11110000;
    state->memory[0x47] = 0b10000000;
    state->memory[0x48] = 0b11110000;
    state->memory[0x49] = 0b10000000;
    state->memory[0x4A] = 0b11110000;
    // f
    state->memory[0x4B] = 0b11110000;
    state->memory[0x4C] = 0b10000000;
    state->memory[0x4D] = 0b11110000;
    state->memory[0x4E] = 0b10000000;
    state->memory[0x4F] = 0b10000000;
}

// initialise a chip-8 instance
Chip8State *InitChip8(void)
{
    Chip8State *s = calloc(sizeof(Chip8State), 1);

    if (s)
    {
        s->memory = calloc(MEMORY_CAPACITY, 1);
        if (s->memory)
        {
            s->screen = &s->memory[DISPLAY_BUFFER];
            s->SP = STACK_BUFFER;
            s->PC = PROGRAM_BUFFER;
            s->I = 0;
            s->awaitingKey = 0;
            memset(s->V, 0x00, 0x10); // init V registers to 0

            InsertFontIntoMemory(s);
        }
    }

    return s;
}

static void Op0(Chip8State *state, uint8_t *instr)
{
    switch (instr[1])
    {
    case 0xe0: // CLS
        // set all bits in the screen area to 0
        memset(state->screen, 0, 256); // (64 / 8) * 32 bytes
        state->PC += 2;
        break;
    case 0xee: // RET
        // take two bytes from the stack
        uint16_t target = (state->memory[state->SP] << 8) | state->memory[state->SP + 1];
        state->SP += 2;
        // set them in the PC
        state->PC = target;
        break;
    default: // SYS $NNN
        // NOT IMPLEMENTED
        break;
    }
}

static void Op8(Chip8State *state, uint8_t *instr)
{
    uint8_t X = instr[0] & 0x0f;
    uint8_t Y = (instr[1] & 0xf0) >> 4;

    switch (instr[1] & 0x0f)
    {
    case 0x0: // MOV VX,VY
        state->V[X] = state->V[Y];
        break;
    case 0x1: // OR VX,VY
        state->V[X] = state->V[X] | state->V[Y];
        break;
    case 0x2: // AND VX,VY
        state->V[X] = state->V[X] & state->V[Y];
        break;
    case 0x3: // XOR VX,VY
        state->V[X] = state->V[X] ^ state->V[Y];
        break;
    case 0x4: // ADD VX,VY
    {
        uint16_t result = state->V[X] + state->V[Y];
        state->V[0xF] = result > 0xFF;
        state->V[X] = result & 0xFF;
    }
    break;
    case 0x5: // SUB VX,VY
        state->V[0xF] = state->V[X] > state->V[Y];
        state->V[X] -= state->V[Y];
        break;
    case 0x6: // RSHFT VX,1
        // save the least significant bit 0b00000001/0x01/001
        state->V[0xF] = state->V[X] & 0b1;
        state->V[X] = (state->V[X] >> 1) & 0x7F;
        break;
    case 0x7: // BSUB VX,VY
        state->V[0xF] = state->V[Y] > state->V[X];
        state->V[X] = state->V[Y] - state->V[X];
        break;
    case 0xe: // LSHFT VX,1
        // save the most significant bit 0b10000000/0x80/128
        state->V[0xF] = (state->V[X] & 0b10000000);
        state->V[X] = (state->V[X] << 1) & 0xFE;
        break;
    }
}

static void OpD(Chip8State *state, uint8_t spr_x, uint8_t spr_y, uint8_t spr_h)
{
    // need to draw each bit individually because pixels are XOR'ed with each other
    // this also determines if a collision has occurred

    // Draw sprite
    int i, j;
    state->V[0xF] = 0;
    for (i = 0; i < spr_h; i++)
    {
        uint8_t *sprite = &state->memory[state->I + i];
        int spritebit = 7;
        for (j = spr_x; j < (spr_x + 8) && j < 64; j++)
        {
            int jover8 = j / 8; // picks the byte in the row
            int jmod8 = j % 8;  // picks the bit in the byte
            uint8_t srcbit = (*sprite >> spritebit) & 0x1;

            if (srcbit)
            {
                uint8_t *destbyte_p = &state->screen[(i + spr_y) * (64 / 8) + jover8];
                uint8_t destbyte = *destbyte_p;
                uint8_t destmask = (0x80 >> jmod8);
                uint8_t destbit = destbyte & destmask;

                srcbit = srcbit << (7 - jmod8);

                if (srcbit & destbit)
                {
                    state->V[0xF] = 1;
                }

                destbit ^= srcbit;

                destbyte = (destbyte & ~destmask) | destbit;

                *destbyte_p = destbyte;
            }
            spritebit--;
        }
    }
}

static void OpE(Chip8State *state, uint8_t *instr)
{
    uint8_t X = instr[0] & 0x0f;
    switch (instr[1])
    {
    case 0x9e: // SKIP.KEY VX
        if (state->keys[state->V[X]])
        {
            state->PC += 2;
        }
        break;
    case 0xa1: // SKIP.NKEY VX
        if (!state->keys[state->V[X]])
        {
            state->PC += 2;
        }
        break;
    }
    state->PC += 2;
}

static void OpF(Chip8State *state, uint8_t *instr)
{
    uint8_t X = instr[0] & 0x0f;

    switch (instr[1])
    {
    case 0x07: // MOV VX,DELAY
        state->V[X] = state->delay;
        state->PC += 2;
        break;
    case 0x0a: // MOV VX,KEY
    {
        printf("%-10i Awaiting key not yet implemented!\n", SDL_GetTicks());
        if (!state->awaitingKey)
        {
            state->awaitingKey = 1;
        }
        else
        {
            // determine if a key is pressed
            uint8_t k;
            for (k = 0; k < 0x10; ++k)
            {
                if (state->keys[k])
                {
                    break;
                }
            }
            // if was pressed:
            if (k < 0x10)
            {
                state->V[X] = k;
                state->awaitingKey = 0;
                state->PC += 2;
            }
            // else, continue and come back
        }
    }
    break;
    case 0x15: // MOV DELAY,VX
        state->delay = state->V[X];
        state->PC += 2;
        break;
    case 0x18: // MOV SOUND,VX
        state->sound = state->V[X];
        state->PC += 2;
        break;
    case 0x1e: // ADD I,VX
        state->I += state->V[X];
        state->PC += 2;
        break;
    case 0x29: // SPRITE.GET I,VX
        // char sprite locations start at addr 0, sprites are 5 tall
        // therefore: address = value * 5
        state->I = state->V[X] * 5;
        state->PC += 2;
        break;
    case 0x33: // BCD VX
    {
        uint8_t value = state->V[X];
        uint8_t ones = value % 10;
        value /= 10;
        uint8_t tens = value % 10;
        uint8_t hundreds = value / 10;
        state->memory[state->I] = hundreds;
        state->memory[state->I + 1] = tens;
        state->memory[state->I + 2] = ones;
        state->PC += 2;
    }
    break;
    case 0x55: // REG.DUMP VX
        for (int v = 0; v <= X; ++v)
        {
            state->memory[state->I + v] = state->V[v];
        }
        state->I += X + 1;
        state->PC += 2;
        break;
    case 0x65: // REG.LOAD VX
        for (int v = 0; v <= X; ++v)
        {
            state->V[v] = state->memory[state->I + v];
        }
        state->I += X + 1;
        state->PC += 2;
        break;
    }
}

// executes the next instruction for the given state
void EmulateChip8(Chip8State *state)
{
    // update timers
    if (state->delay)
        state->delay--;
    if (state->sound)
        state->sound--;

    uint8_t *instr = &state->memory[state->PC];

    uint8_t highNibble = (instr[0] & 0xF0) >> 4;
    // second nibble
    uint8_t X = instr[0] & 0x0F;
    // third nibble
    uint8_t Y = ((instr[1] & 0xF0) >> 4);
    // fourth nibble
    uint8_t N = instr[1] & 0x0F;
    // second byte (3rd,4th nibbles)
    uint8_t NN = instr[1];
    // 2nd,3rd,4th nibbles
    uint16_t NNN = (((instr[0] & 0x0F) << 8) | instr[1]) & 0x0FFF;

    switch (highNibble)
    {
    case 0x0: // Op0
        Op0(state, instr);
        // Op0 adjusts PC itself, so don't here
        break;
    case 0x1: // JMP $NNN
        // jump to current instruction - infinite loop
        if (state->PC == NNN)
        {
            // TODO: halt on infinite loop
            printf("%-10i Infinite loop detected!\n", SDL_GetTicks());
        }
        state->PC = NNN;
        break;
    case 0x2: // CALL $NNN
        state->SP -= 2;
        // load the two-byte return address from the next memory index after this instruction into 2 bytes in the stack
        state->memory[state->SP] = ((state->PC + 2) & 0xFF00) >> 8;
        state->memory[state->SP + 1] = (state->PC + 2) & 0xFF;
        // set PC to intended address
        state->PC = NNN;
        break;
    case 0x3: // SKIP.EQ VX,#$NN
        if (state->V[X] == NN)
        {
            state->PC += 2;
        }
        state->PC += 2;
        break;
    case 0x4: // SKIP.NE VX,#$NN
        if (state->V[X] != NN)
        {
            state->PC += 2;
        }
        state->PC += 2;
        break;
    case 0x5: // SKIP.EQ VX,VY
        if (state->V[X] == state->V[Y])
        {
            state->PC += 2;
        }
        state->PC += 2;
        break;
    case 0x6: // MOV VX,#$NN
        state->V[X] = NN;
        state->PC += 2;
        break;
    case 0x7: // ADD VX,#$NN
        state->V[X] += NN;
        state->PC += 2;
        break;
    case 0x8: // Op8
        Op8(state, instr);
        state->PC += 2;
        break;
    case 0x9: // SKIP.NE VX,VY
        if (state->V[X] != state->V[Y])
        {
            state->PC += 2;
        }
        state->PC += 2;
        break;
    case 0xa: // MOV I,#$NNN
        state->I = NNN;
        state->PC += 2;
        break;
    case 0xb: // JUMP $NNN+V0
        state->PC = NNN + (uint16_t)state->V[0];
        break;
    case 0xc: // RANDMASK VX,$NN
        state->V[X] = rand() & (uint32_t)NN;
        state->PC += 2;
        break;
    case 0xd: // DRAW VX,VY,#$N
        OpD(state, state->V[X], state->V[Y], N);
        state->PC += 2;
        break;
    case 0xe: // OpE
        OpE(state, instr);
        // OpE adjusts PC itself, so don't here
        break;
    case 0xf: // OpF
        OpF(state, instr);
        // OpF adjusts PC itself, so don't here
        break;
    }
}
