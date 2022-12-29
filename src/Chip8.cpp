#include "Chip8.h"

#define EXTRACT_X \
    uint8_t Vx = (opcode & 0x0F00) >> 8;

#define EXTRACT_XY \
    EXTRACT_X \
    uint8_t Vy = (opcode & 0x00F0) >> 4;

#define EXTRACT_NNN \
    uint16_t addr = opcode & 0x0FFF;

#define EXTRACT_KK \
    uint8_t byte = opcode & 0x00FF;


Chip8::Chip8() : randGen(std::chrono::system_clock::now().time_since_epoch().count()) {
    // init pc to beginning of rom
    pc = START_ADDRESS;

    // load_rom fontset into memory
    memcpy(&memory[FONTSET_START_ADDRESS], FONTSET, FONTSET_SIZE);

    // function pointer table
    fill(begin(table0), end(table0), &Chip8::OP_NULL);
    fill(begin(table8), end(table8), &Chip8::OP_NULL);
    fill(begin(tableE), end(tableE), &Chip8::OP_NULL);
    fill(begin(tableF), end(tableF), &Chip8::OP_NULL);

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

void Chip8::load_rom(string filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();

    if (size == -1) {
        std::cerr << "Error loading file" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    file.seekg(0, std::ios::beg);
    file.read((char *) &memory[START_ADDRESS], size);
    file.close();
}

void Chip8::cycle() {
    // fetch
    opcode = (memory[pc] << 8u) | memory[pc + 1];
    pc += 2;

    // decode + execute
    ((*this).*(table[opcode >> 12u]))();

//    printf("%08x %08x\n", opcode >> 12, opcode);

    // decrement delayTimer if set
    if (delayTimer > 0)
        --delayTimer;

    // decrement soundTimer if set
    if (soundTimer > 0)
        --soundTimer;
}

void Chip8::Table0() { ((*this).*(table0[opcode & 0x000F]))(); }

void Chip8::Table8() { ((*this).*(table8[opcode & 0x000F]))(); }

void Chip8::TableE() { ((*this).*(tableE[opcode & 0x000F]))(); }

void Chip8::TableF() { ((*this).*(tableF[opcode & 0x00FF]))(); }

void Chip8::OP_NULL() {
    fprintf(stderr, "OP_NULL: %08x\n", opcode);
}


// 00E0 - CLS
void Chip8::OP_00E0() {
    fill(begin(video), end(video), 0);
}

// 00EE - RET
void Chip8::OP_00EE() {
    sp--;
    pc = stack[sp];
}

// 1nnn - JP addr
void Chip8::OP_1nnn() {
    EXTRACT_NNN
    pc = addr;
}

// 2nnn - CALL addr
void Chip8::OP_2nnn() {
    EXTRACT_NNN
    stack[sp] = pc;
    sp++;
    pc = addr;
}

// 3xkk - SE Vx, byte
void Chip8::OP_3xkk() {
    EXTRACT_X
    EXTRACT_KK
    if (registers[Vx] == byte) pc += 2;
}

// 4xkk - SNE Vx, byte
void Chip8::OP_4xkk() {
    EXTRACT_X
    EXTRACT_KK
    if (registers[Vx] != byte) pc += 2;
}

// 5xy0 - SE Vx, Vy
void Chip8::OP_5xy0() {
    EXTRACT_XY
    if (registers[Vx] == registers[Vy]) pc += 2;
}

// 6xkk - LD Vx, byte
void Chip8::OP_6xkk() {
    EXTRACT_X
    EXTRACT_KK
    registers[Vx] = byte;
}

// 7xkk - ADD Vx, byte
void Chip8::OP_7xkk() {
    EXTRACT_X
    EXTRACT_KK
    registers[Vx] += byte;
}

// 8xy0 - LD Vx, Vy
void Chip8::OP_8xy0() {
    EXTRACT_XY
    registers[Vx] = registers[Vy];
}

// 8xy1 - OR Vx, Vy
void Chip8::OP_8xy1() {
    EXTRACT_XY
    registers[Vx] |= registers[Vy];
}

// 8xy2 - AND Vx, Vy
void Chip8::OP_8xy2() {
    EXTRACT_XY
    registers[Vx] &= registers[Vy];
}

// 8xy3 - XOR Vx, Vy
void Chip8::OP_8xy3() {
    EXTRACT_XY
    registers[Vx] ^= registers[Vy];
}

// 8xy4 - ADD Vx, Vy
void Chip8::OP_8xy4() {
    EXTRACT_XY
    uint16_t sum = registers[Vx] + registers[Vy];

    // set carry bit
    registers[0xF] = sum >> 8u;

    // implicitly truncate carry bit
    registers[Vx] = sum;
}

// 8xy5 - SUB Vx, Vy
void Chip8::OP_8xy5() {
    EXTRACT_XY

    registers[0xF] = registers[Vx] > registers[Vy];
    registers[Vx] = (uint8_t) registers[Vx] > (uint8_t) registers[Vy];
}

// 8xy6 - SHR Vx {, Vy}
void Chip8::OP_8xy6() {
    EXTRACT_XY
    registers[Vx] = registers[Vy];
    registers[0xF] = registers[Vx] & 1;
    registers[Vx] = (registers[Vx] >> 1) & 0x0FFF;
}

// 8xy7 - SUBN Vx, Vy
void Chip8::OP_8xy7() {
    EXTRACT_XY
    registers[0xF] = registers[Vy] > registers[Vx];
    registers[Vx] = (uint8_t) registers[Vy] > (uint8_t) registers[Vx];
}

// 8xyE - SHL Vx {, Vy}
void Chip8::OP_8xyE() {
    EXTRACT_XY
    registers[Vx] = registers[Vy];
    registers[0xF] = (registers[Vx] >> 11) & 1;
    registers[Vx] = (registers[Vx] << 1) & 0x0FFF;
}

// 9xy0 - SNE Vx, Vy
void Chip8::OP_9xy0() {
    EXTRACT_XY
    if (registers[Vx] != registers[Vy]) pc += 2;
}

// Annn - LD I, addr
void Chip8::OP_Annn() {
    EXTRACT_NNN
    index = addr;
}

// Bnnn - JP V0, addr
void Chip8::OP_Bnnn() {
    EXTRACT_NNN
    pc = registers[0x0] + addr;
}

// Cxkk - RND Vx, byte
void Chip8::OP_Cxkk() {
    EXTRACT_X
    EXTRACT_KK
    registers[Vx] = randByte(randGen) & byte;
}

// Dxyn - DRW Vx, Vy, nibble
void Chip8::OP_Dxyn() {
    EXTRACT_XY
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
    EXTRACT_X
    uint8_t key = registers[Vx];
    if (keypad[key]) pc += 2;
}

// ExA1 - SKNP Vx
void Chip8::OP_ExA1() {
    EXTRACT_X
    uint8_t key = registers[Vx];
    if (!keypad[key]) pc += 2;
}

// Fx07 - LD Vx, DT
void Chip8::OP_Fx07() {
    EXTRACT_X
    registers[Vx] = delayTimer;
}

// Fx0A - LD Vx, K
void Chip8::OP_Fx0A() {
    EXTRACT_X

    for (int i = 0; i < 16; i++) {
        if (keypad[i]) {
            registers[Vx] = i;
            return;
        }
    }

    pc -= 2;
}

// Fx15 - LD DT, Vx
void Chip8::OP_Fx15() {
    EXTRACT_X
    delayTimer = registers[Vx];
}

// Fx18 - LD ST, Vx
void Chip8::OP_Fx18() {
    EXTRACT_X
    soundTimer = registers[Vx];
}

// Fx1E - ADD I, Vx
void Chip8::OP_Fx1E() {
    EXTRACT_X
    index += registers[Vx];
}

// Fx29 - LD F, Vx
void Chip8::OP_Fx29() {
    EXTRACT_X
    uint16_t digit = registers[Vx];

    index = FONTSET_START_ADDRESS + digit * 5;
}

// Fx33 - LD B, Vx
void Chip8::OP_Fx33() {
    EXTRACT_X
    uint16_t value = registers[Vx];

    memory[index + 2] = value % 10;
    value /= 10;
    memory[index + 1] = value % 10;
    value /= 10;
    memory[index] = value % 10;
}

// Fx55 - LD [I], Vx
void Chip8::OP_Fx55() {
    EXTRACT_X

    for (int v = 0; v <= Vx; v++) {
        memory[index + v] = registers[v];
    }
}

// Fx65 - LD Vx, [I]
void Chip8::OP_Fx65() {
    EXTRACT_X

    for (int v = 0; v <= Vx; v++) {
        registers[v] = memory[index + v];
    }
}



