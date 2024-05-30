#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * CHIP-8 Instruction Set
 * Notes about instructions.
 * Register placeholders
 *   X and Y are used in place of register numbers, e.g. in 6XNN the second
 *    nibble determines which register it used (registers are 0-F).
 * Immediate values
 *   N is used where the number is interpreted as immediate and should be used as-is.
 *   Two (NN) are a byte of data, and three (NNN) make an address.
 */

// Handy regex to search for e.g. 8xxx operations:
// ` 8[0-9a-f] [0-9a-f]{2} `

void disassembleChip8(uint8_t *program, int pc)
{
    uint8_t *instructionCode = &program[pc];
    uint8_t firstNibble = (instructionCode[0] >> 4);
    // second nibble
    uint8_t X = instructionCode[0] & 0x0f;
    // third nibble
    uint8_t Y = (instructionCode[1] >> 4);
    // fourth nibble
    uint8_t N = instructionCode[1] & 0x0f;
    // second byte (3rd,4th nibbles)
    uint8_t NN = instructionCode[1];
    // 2nd,3rd,4th nibbles
    uint16_t NNN = ((instructionCode[0] << 8) + instructionCode[1]) & 0x0fff;
    // handy command storage for switches
    char *cmd;

    printf("%04x %02x %02x ", pc, instructionCode[0], instructionCode[1]);

    switch (firstNibble)
    {
    case 0x0:
        if (*instructionCode == 0x00e0)
        {
            /**
             * 00e0
             * Clears the screen
             */
            printf("%-10s", "CLS");
        }
        else if (*instructionCode == 0x00ee)
        {
            /**
             * 00ee
             * Returns from a subroutine
             * `return;`
             */
            printf("%-10s", "RTN");
        }
        else
        {
            /**
             * 0NNN
             * Calls machine code routine at address NNN
             * `return;`
             */
            printf("%-10s $%03x", "CMC", NNN);
        }
        break;

    case 0x1:
        /**
         * 1NNN
         * Jump to address NNN
         * `goto NNN;`
         */
        printf("%-10s $%03x", "JMP", NNN);
        break;

    case 0x2:
        /**
         * 2NNN
         * Calls subroutine at address NNN
         * `*(0xNNN)()`
         */
        printf("%-10s $%03x", "CALL", NNN);
        break;

    case 0x3:
        /**
         * 3XNN
         * Skip next instruction if VX equals NN
         * `if (Vx == NN)`
         */
        printf("%-10s V%01x,#$%02x", "SKIP.EQ", X, NN);
        break;

    case 0x4:
        /**
         * 4XNN
         * Skip next instruction if VX does not equal NN
         * `if (Vx != NN)`
         */
        printf("%-10s V%01x,#$%02x", "SKIP.NE", X, NN);
        break;

    case 0x5:
        /**
         * 5XY0
         * Skip next instruction if VX equals VY
         * `if (Vx == Vy)`
         */
        printf("%-10s V%01x,V%01x", "SKIP.EQ", X, Y);
        break;

    case 0x6:
        /**
         * 6XNN
         * Sets VX to NN
         */
        printf("%-10s V%01X,#$%02x", "MOV", X, NN);
        break;

    case 0x7:
        /**
         * 7XNN
         * Adds NN to VX (carry flag is not changed)
         * `Vx += NN`
         */
        printf("%-10s V%01x,#$%02x", "ADD", X, NN);
        break;

    case 0x8:
        /**
         * 8XY*
         * Bit and arithmetic operations, determined by the 4th nibble (*)
         */
        switch (N)
        {
        case 0:
            // Sets VX to the value of VY
            // `Vx = Vy`
            cmd = "MOV";
            break;
        case 1:
            // Sets VX to "VX or VY"
            // `Vx |= Vy`
            cmd = "OR";
            break;
        case 2:
            // Sets VX to "VX and VY"
            // `Vx &= Vy`
            cmd = "AND";
            break;
        case 3:
            // Sets VX to "VX xor VY"
            // `Vx ^= Vy`
            cmd = "XOR";
            break;
        case 4:
            // Adds VY to VX. VF is set to 1 when there's an overflow, and 0 when there is not.
            // `Vx += Vy`
            cmd = "ADD";
            break;
        case 5:
            // Subtracts VY from VX. VF is set to 0 when there's an underflow, and 1 when there is not (i.e. VF set to VX >= VY)
            // `Vx -= Vy`
            cmd = "SUB";
            break;
        case 6:
            // Stores the least significant bit of VX in VF, then shifts VX to the right by 1
            // `Vx >>= 1`
            cmd = "RSHFT";
            break;
        case 7:
            // Sets VX to VY subtract VX. VF is set to 0 when there's an
            //  underflow, and 1 when there is not (i.e. VF set to VY >= VX)
            //  (backward subtract)
            // `Vx = Vy - Vx`
            cmd = "BSUB";
            break;
        case 0xe:
            // Stores the most significant bit of VX in VF, then shifts VX to the left by 1
            // `Vx <<= 1`
            cmd = "LSHFT";
            break;
        default:
            cmd = "UNKNOWN 8";
            break;
        }
        printf("%-10s V%01x,V%01x", cmd, X, Y);
        break;

    case 0x9:
        /**
         * 9XY0
         * Skip next instruction if VX does not equal VY
         * `if (Vx != Vy)`
         */
        printf("%-10s V%01x,V%01x", "SKIP.NE", X, Y);
        break;

    case 0xa:
        /**
         * ANNN
         * Sets I to the address NNN
         * `I = NNN`
         */
        printf("%-10s I,$%03x", "MVI", NNN);
        break;

    case 0xb:
        /**
         * BNNN
         * Jumps to the address NNN plus V0
         * `PC = V0 + NNN`
         */
        printf("%-10s V0+$%03x", "JUMP", NNN);
        break;

    case 0xc:
        /**
         * CXNN
         * Sets VX to the result of a bitwise and operation on a random number
         *  (Typically: 0 to 255) and NN
         * `Vx = rand() & NN`
         */
        printf("%-10s V%01x,#$%02x", "RANDMASK", X, NN);
        break;

    case 0xd:
        /**
         * DXYN
         * Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels
         *  and a height of N pixels. Each row of 8 pixels is read as bit-coded
         *  starting from memory location I; I value does not change after the
         *  execution of this instruction. As described above, VF is set to 1 if
         *  any screen pixels are flipped from set to unset when the sprite is
         *  drawn, and to 0 if that does not happen.
         * `draw(Vx, Vy, N)`
         */
        printf("%-10s V%01x,V%01x,#$%01x", "DRAW", X, Y, N);
        break;

    case 0xe:
        switch (NN)
        {
        case 0x9e:
            /**
             * EX9E
             * Skips the next instruction if the key stored in VX is pressed
             *  (usually the next instruction is a jump to skip a code block)
             * `if (key() == Vx)`
             */
            cmd = "SKIP.KEY";
            break;

        case 0xa1:
            /**
             * EXA1
             * Skips the next instruction if the key stored in VX is not pressed
             *  (usually the next instruction is a jump to skip a code block)
             * `if (key() != Vx)`
             */
            cmd = "SKIP.NKEY";
            break;

        default:
            cmd = "UNKNOWN E";
            break;
        }
        printf("%-10s V%01x", cmd, X);
        break;

    case 0xf:
        switch (NN)
        {
        case 0x07:
            /**
             * FX07
             * Sets VX to the value of the delay timer
             * `Vx = get_delay()`
             */
            cmd = "DELAY.GET";
            break;

        case 0x0a:
            /**
             * FX0A
             * A key press is awaited, and then stored in VX
             *  (blocking operation, all instruction halted until next key event)
             * `Vx = get_key()`
             */
            cmd = "KEY.GET";
            break;

        case 0x15:
            /**
             * FX15
             * Sets the delay timer to VX
             * `set_delay(Vx)`
             */
            cmd = "DELAY.SET";
            break;

        case 0x18:
            /**
             * FX18
             * Sets the sound time to VX
             * `set_sound(Vx)`
             */
            cmd = "SOUND.SET";
            break;

        case 0x1e:
            /**
             * FX1E
             * Adds VX to I. VF is not affected
             * `I += Vx`
             */
            cmd = "I.ADD";
            break;

        case 0x29:
            /**
             * FX29
             * Sets I to the location of the sprite for the character in VX.
             *  Characters 0-F (in hexadecimal) are represented by a 4x5 font
             * `I = sprite_addr(Vx)`
             */
            cmd = "SPRITE.GET";
            break;

        case 0x33:
            /**
             * FX33
             * Stores the binary-coded decimal representation of VX, with the
             *  hundreds digit in memory at location in I, the tens digit at
             *  location I+1, and the ones digit at location I+2
             * `
             *  set_BCD(Vx)
             *  *(I+0) = BCD(3);
             *  *(I+1) = BCD(2);
             *  *(I+2) = BCD(1);
             * `
             */
            cmd = "BCD";
            break;

        case 0x55:
            /**
             * FX55
             * Stores from V0 to VX (including VX) in memory, starting at
             *  address I. The offset from I is increased by 1 for each value
             *  written, but I itself is left unmodified
             * `reg_dump(Vx, &I)`
             */
            cmd = "REG.DUMP";
            break;

        case 0x65:
            /**
             * FX65
             * Fills from V0 to VX (including VX) with values from memory,
             *  starting at address I. The offset from I is increased by 1 for
             *  each value read, but I itself is left unmodified
             * `reg_load(Vx, &I)`
             */
            cmd = "REG.LOAD";
            break;

        default:
            cmd = "UNKNOWN F";
            break;
        }
        printf("%-10s V%01x", cmd, X);
        break;

    default:
        printf("%-10s %04x", "UNKNOWN", (instructionCode[0] << 8) + instructionCode[1]);
        break;
    }
}

int disassemble_main(int argc, char **argv)
{
    // check args
    if (argc != 2)
    {
        printf("ERROR: Must specify file to disassemble\n");
        return 1;
    }

    // read file
    FILE *file = fopen(argv[1], "rb");
    if (!file)
    {
        printf("ERROR: Couldn't open %s\n", argv[1]);
        return 2;
    }

    // get file size
    fseek(file, 0L, SEEK_END);
    int fsize = ftell(file);
    fseek(file, 0L, SEEK_SET);

    // CHIP-8 convention puts programs into memory at 0x200
    // ROMs will be hardcoded to expect that
    // read the file into memory at 0x200 and close it
    unsigned char *memory = malloc(fsize + 0x200);
    fread(memory + 0x200, fsize, 1, file);
    fclose(file);

    // init registers
    int pc = 0x200;
    // disassemble program
    while (pc < (fsize + 0x200))
    {
        disassembleChip8(memory, pc);
        pc += 2; // commands are 2 bytes
        printf("\n");
    }

    return 0;
}
