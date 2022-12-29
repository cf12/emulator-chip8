//
// Created by cf12 on 12/28/22.
//

#include "Chip8.h"

void Chip8::LoadROM(string filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();

    if (size == -1) {
        std::cerr << "Error loading file" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    file.seekg(0, std::ios::beg);
    file.read((char *) &memory[START_ADDRESS], size);
    file.close();
};

Chip8::Chip8() : randGen(std::chrono::system_clock::now().time_since_epoch().count()) {
    // init pc to beginning of rom
    pc = START_ADDRESS;

    // load fontset into memory
    memcpy(&memory[FONTSET_START_ADDRESS], FONTSET, FONTSET_SIZE);

    // Set up function pointer table
    table[0x0] = &Chip8::Table0;
    table[0x1] = &Chip8::OP_1nnn;
    table[0x2] = &Chip8::OP_2nnn;
    table[0x3] = &Chip8::OP_3xkk;
    table[0x4] = &Chip8::OP_4xkk;
    table[0x5] = &Chip8::OP_5xy0;
    table[0x6] = &Chip8::OP_6xkk;
    table[0x7] = &Chip8::OP_7xkk;
    table[0x8] = &Chip8::Table8;
    table[0x9] = &Chip8::OP_9xy0;
    table[0xA] = &Chip8::OP_Annn;
    table[0xB] = &Chip8::OP_Bnnn;
    table[0xC] = &Chip8::OP_Cxkk;
    table[0xD] = &Chip8::OP_Dxyn;
    table[0xE] = &Chip8::TableE;
    table[0xF] = &Chip8::TableF;

    for (size_t i = 0; i <= 0xE; i++) {
        table0[i] = &Chip8::OP_NULL;
        table8[i] = &Chip8::OP_NULL;
        tableE[i] = &Chip8::OP_NULL;
    }

    table0[0x0] = &Chip8::OP_00E0;
    table0[0xE] = &Chip8::OP_00EE;

    table8[0x0] = &Chip8::OP_8xy0;
    table8[0x1] = &Chip8::OP_8xy1;
    table8[0x2] = &Chip8::OP_8xy2;
    table8[0x3] = &Chip8::OP_8xy3;
    table8[0x4] = &Chip8::OP_8xy4;
    table8[0x5] = &Chip8::OP_8xy5;
    table8[0x6] = &Chip8::OP_8xy6;
    table8[0x7] = &Chip8::OP_8xy7;
    table8[0xE] = &Chip8::OP_8xyE;

    tableE[0x1] = &Chip8::OP_ExA1;
    tableE[0xE] = &Chip8::OP_Ex9E;

    for (size_t i = 0; i <= 0x65; i++) {
        tableF[i] = &Chip8::OP_NULL;
    }

    tableF[0x07] = &Chip8::OP_Fx07;
    tableF[0x0A] = &Chip8::OP_Fx0A;
    tableF[0x15] = &Chip8::OP_Fx15;
    tableF[0x18] = &Chip8::OP_Fx18;
    tableF[0x1E] = &Chip8::OP_Fx1E;
    tableF[0x29] = &Chip8::OP_Fx29;
    tableF[0x33] = &Chip8::OP_Fx33;
    tableF[0x55] = &Chip8::OP_Fx55;
    tableF[0x65] = &Chip8::OP_Fx65;
}

void Chip8::Table0() { ((*this).*(table0[opcode & 0x000Fu]))(); }

void Chip8::Table8() { ((*this).*(table8[opcode & 0x000Fu]))(); }

void Chip8::TableE() { ((*this).*(tableE[opcode & 0x000Fu]))(); }

void Chip8::TableF() { ((*this).*(tableF[opcode & 0x00FFu]))(); }

void Chip8::OP_NULL() {}


// 00E0 - CLS
void Chip8::OP_00E0() { memset(video, 0, sizeof(video)); }

// RET
void Chip8::OP_00EE() {
    sp--;
    pc = stack[sp];
}

// 1nnn - JP addr
void Chip8::OP_1nnn() {
    uint16_t addr = opcode & 0x0FFF;

    pc = addr;
}

// CALL addr
void Chip8::OP_2nnn() {
    uint16_t addr = opcode & 0x0FFF;

    stack[sp] = pc;
    sp++;
    pc = addr;
}

// SE Vx, byte
void Chip8::OP_3xkk() {
    uint8_t Vx = opcode >> 8;
    uint8_t k = opcode & 0x00FF;

    if (registers[Vx] == k) pc += 2;
}

// SNE Vx, byte
void Chip8::OP_4xkk() {
    uint8_t Vx = opcode >> 8;
    uint8_t byte = opcode & 0x00FFu;

    if (registers[Vx] != byte) pc += 2;
}

// SE Vx, Vy
void Chip8::OP_5xy0() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] == registers[Vy]) pc += 2;
}

// 6xkk - LD Vx, byte
void Chip8::OP_6xkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] = byte;
}

// 7xkk - ADD Vx, byte
void Chip8::OP_7xkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] += byte;
}

// 8xy0 - LD Vx, Vy
void Chip8::OP_8xy0() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] = registers[Vy];
}

// 8xy1 - OR Vx, Vy
void Chip8::OP_8xy1() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] |= registers[Vy];
}

// 8xy2 - AND Vx, Vy
void Chip8::OP_8xy2() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] &= registers[Vy];
}

// 8xy3 - XOR Vx, Vy
void Chip8::OP_8xy3() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] ^= registers[Vy];
}

// 8xy4 - ADD Vx, Vy
void Chip8::OP_8xy4() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    uint16_t sum = registers[Vx] + registers[Vy];

    // set carry bit
    registers[0xF] = sum >> 8u;

    // implicitly truncate carry bit
    registers[Vx] = sum;
}

// 8xy5 - SUB Vx, Vy
void Chip8::OP_8xy5() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    uint16_t diff = registers[Vx] - registers[Vy];
    // if Vx > Vy = Vx - Vy > 0, then we set borrow bit
    registers[0xF] = diff > 0;
    registers[Vx] = diff;
}

// 8xy6 - SHR Vx {, Vy}
void Chip8::OP_8xy6() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[0xF] = registers[Vx] & 1;
    registers[Vx] >>= 1;
}

// 8xy7 - SUBN Vx, Vy
void Chip8::OP_8xy7() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    uint16_t diff = registers[Vy] - registers[Vx];
    // if Vy > Vx = Vy - Vx > 0, then we set borrow bit
    registers[0xF] = diff > 0;
    registers[Vx] = diff;
}

// 8xyE - SHL Vx {, Vy}
void Chip8::OP_8xyE() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[0xF] = (registers[Vx] >> 11) & 1;
    registers[Vx] <<= 1;
}

// 9xy0 - SNE Vx, Vy
void Chip8::OP_9xy0() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] != registers[Vy]) pc += 2;
}

// Annn - LD I, addr
void Chip8::OP_Annn() {
    uint16_t addr = opcode & 0x0FFF;

    index = addr;
}

// Bnnn - JP V0, addr
void Chip8::OP_Bnnn() {
    uint16_t addr = opcode & 0x0FFF;

    pc = registers[0x0] + addr;
}

// Cxkk - RND Vx, byte
void Chip8::OP_Cxkk() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t byte = opcode & 0x00FF;

    registers[Vx] = randByte(randGen) & byte;
}

// Dxyn - DRW Vx, Vy, nibble
void Chip8::OP_Dxyn() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    uint8_t height = opcode & 0x000Fu;

    uint8_t x = registers[Vx] % VIDEO_WIDTH;
    uint8_t y = registers[Vy] % VIDEO_HEIGHT;

    registers[0xF] = 0;

    for (int dy = 0; dy < height; dy++) {
        uint8_t sprite = memory[index + dy];

        for (int dx = 0; dx < 8; dx++) {
            uint8_t sprite_pixel = (sprite >> (7 - dx)) & 1;
            uint32_t *video_pixel = &video[(y + dy) * VIDEO_WIDTH + (x + dx)];

            if (sprite_pixel) {
                if (*video_pixel) {
                    registers[0xF] = 1;
                }

                *video_pixel ^= 0xFFFFFFFF;
            }
        }
    }
}

// Ex9E - SKP Vx
void Chip8::OP_Ex9E() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t key = registers[Vx];
    if (keypad[key]) pc += 2;
}

// ExA1 - SKNP Vx
void Chip8::OP_ExA1() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t key = registers[Vx];
    if (!keypad[key]) pc += 2;
}

// Fx07 - LD Vx, DT
void Chip8::OP_Fx07() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;

    registers[Vx] = delayTimer;
}

// Fx0A - LD Vx, K
void Chip8::OP_Fx0A() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;

    if (keypad[0])
        registers[Vx] = 0;
    else if (keypad[1])
        registers[Vx] = 1;
    else if (keypad[2])
        registers[Vx] = 2;
    else if (keypad[3])
        registers[Vx] = 3;
    else if (keypad[4])
        registers[Vx] = 4;
    else if (keypad[5])
        registers[Vx] = 5;
    else if (keypad[6])
        registers[Vx] = 6;
    else if (keypad[7])
        registers[Vx] = 7;
    else if (keypad[8])
        registers[Vx] = 8;
    else if (keypad[9])
        registers[Vx] = 9;
    else if (keypad[10])
        registers[Vx] = 10;
    else if (keypad[11])
        registers[Vx] = 11;
    else if (keypad[12])
        registers[Vx] = 12;
    else if (keypad[13])
        registers[Vx] = 13;
    else if (keypad[14])
        registers[Vx] = 14;
    else if (keypad[15])
        registers[Vx] = 15;
    else
        pc -= 2;
}

// Fx15 - LD DT, Vx
void Chip8::OP_Fx15() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;

    delayTimer = registers[Vx];
}

// Fx18 - LD ST, Vx
void Chip8::OP_Fx18() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;

    soundTimer = registers[Vx];
}

// Fx1E - ADD I, Vx
void Chip8::OP_Fx1E() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;

    index += registers[Vx];
}

// Fx29 - LD F, Vx
void Chip8::OP_Fx29() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint16_t digit = registers[Vx];

    index = FONTSET_START_ADDRESS + digit * 5;
}

// Fx33 - LD B, Vx
void Chip8::OP_Fx33() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint16_t value = registers[Vx];

    memory[index + 2] = value % 10;
    index /= 10;
    memory[index + 1] = value % 10;
    index /= 10;
    memory[index] = value % 10;
}

// Fx55 - LD [I], Vx
void Chip8::OP_Fx55() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;

    for (int v = 0; v <= Vx; v++) {
        memory[index + v] = registers[v];
    }
}

// Fx65 - LD Vx, [I]
void Chip8::OP_Fx65() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;

    for (int v = 0; v <= Vx; v++) {
        registers[v] = memory[index + v];
    }
}

void Chip8::Cycle() {
    // Fetch
    opcode = (memory[pc] << 8u) | memory[pc + 1];
    pc += 2;

    // Decode and Execute
//    ((*this).*(table[(opcode & 0xF000u) >> 12u]))();

    printf("%08x %08x\n", opcode >> 12, opcode);

    switch (opcode >> 12) {
        case 0x0:
            switch (opcode) {
                case (0x00E0): OP_00E0(); break;
                case (0x00EE): OP_00EE(); break;
            }
        case 0x1: OP_1nnn(); break;
        case 0x2: OP_2nnn(); break;
        case 0x3: OP_3xkk(); break;
        case 0x4: OP_4xkk(); break;
        case 0x5: OP_5xy0(); break;
        case 0x6: OP_6xkk(); break;
        case 0x7: OP_7xkk(); break;
        case 0x8:
            switch (opcode & 0x000F) {
                case 0x0: OP_8xy0(); break;
                case 0x1: OP_8xy1(); break;
                case 0x2: OP_8xy2(); break;
                case 0x3: OP_8xy3(); break;
                case 0x4: OP_8xy4(); break;
                case 0x5: OP_8xy5(); break;
            }
        case 0x9: OP_9xy0(); break;
        case 0xA: OP_Annn(); break;
        case 0xB: OP_Bnnn(); break;
        case 0xC: OP_Cxkk(); break;
        case 0xD: OP_Dxyn(); break;
        case 0xE:
            switch (opcode & 0x00FF) {
                case 0x9E: OP_Ex9E(); break;
                case 0xA1: OP_ExA1(); break;
            }
        case 0xF:
            switch (opcode & 0x00FF) {
                case 0x07: OP_Fx07(); break;
                case 0x15: OP_Fx15(); break;
                case 0x18: OP_Fx18(); break;
                case 0x1E: OP_Fx1E(); break;
                case 0x0A: OP_Fx0A(); break;
                case 0x29: OP_Fx29(); break;
                case 0x33: OP_Fx33(); break;
                case 0x55: OP_Fx55(); break;
                case 0x65: OP_Fx65(); break;
            }
        default:
            fprintf(stderr, "INVALID INSTRUCTION: %x\n", opcode);
    }

    // Decrement the delay timer if it's been set
    if (delayTimer > 0)
        --delayTimer;

    // Decrement the sound timer if it's been set
    if (soundTimer > 0)
        --soundTimer;
}


