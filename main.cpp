#include <SDL.h>

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace std;

const unsigned int START_ADDRESS = 0x200;

const unsigned int FONTSET_START_ADDRESS = 0x50;
const unsigned int FONTSET_SIZE = 5 * 16;  // 80
const uint8_t FONTSET[FONTSET_SIZE] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
    0x20, 0x60, 0x20, 0x20, 0x70,  // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
    0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
    0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
    0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
    0xF0, 0x80, 0xF0, 0x80, 0x80   // F
};

const unsigned int VIDEO_WIDTH = 64;
const unsigned int VIDEO_HEIGHT = 32;

class Chip8 {
 public:
  uint8_t registers[16]{};
  uint8_t memory[4096]{};
  uint16_t index{};
  uint16_t pc{};
  uint16_t stack[16]{};
  uint8_t sp{};
  uint8_t delayTimer{};
  uint8_t soundTimer{};
  uint8_t keypad[16]{};
  uint32_t video[64 * 32]{};
  uint16_t opcode{};

  std::default_random_engine randGen;
  std::uniform_int_distribution<uint8_t> randByte;

  void LoadROM(string filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();

    if (size == -1) {
      std::cerr << "Error loading file" << std::endl;
      std::exit(EXIT_FAILURE);
    }

    file.seekg(0, std::ios::beg);
    file.read((char*)&memory[START_ADDRESS], size);
    file.close();
  };

  Chip8()
      : randGen(std::chrono::system_clock::now().time_since_epoch().count()) {
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

  void Table0() { ((*this).*(table0[opcode & 0x000Fu]))(); }
  void Table8() { ((*this).*(table8[opcode & 0x000Fu]))(); }
  void TableE() { ((*this).*(tableE[opcode & 0x000Fu]))(); }
  void TableF() { ((*this).*(tableF[opcode & 0x00FFu]))(); }
  void OP_NULL() {}

  typedef void (Chip8::*Chip8Func)();
  Chip8Func table[0xF + 1];
  Chip8Func table0[0xE + 1];
  Chip8Func table8[0xE + 1];
  Chip8Func tableE[0xE + 1];
  Chip8Func tableF[0x65 + 1];

  // 00E0 - CLS
  void OP_00E0() { memset(video, 0, sizeof(video)); }

  // RET
  void OP_00EE() {
    sp--;
    pc = stack[sp];
  }

  // 1nnn - JP addr
  void OP_1nnn() {
    uint16_t addr = opcode & 0x0FFF;

    pc = addr;
  }

  // CALL addr
  void OP_2nnn() {
    uint16_t addr = opcode & 0x0FFF;

    stack[sp] = pc;
    sp++;
    pc = addr;
  }

  // SE Vx, byte
  void OP_3xkk() {
    uint8_t Vx = opcode >> 8;
    uint8_t k = opcode & 0x00FF;

    if (registers[Vx] == k) pc += 2;
  }

  // SNE Vx, byte
  void OP_4xkk() {
    uint8_t Vx = opcode >> 8;
    uint8_t byte = opcode & 0x00FFu;

    if (registers[Vx] != byte) pc += 2;
  }

  // SE Vx, Vy
  void OP_5xy0() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] == registers[Vy]) pc += 2;
  }

  // 6xkk - LD Vx, byte
  void OP_6xkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] = byte;
  }

  // 7xkk - ADD Vx, byte
  void OP_7xkk() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] += byte;
  }

  // 8xy0 - LD Vx, Vy
  void OP_8xy0() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] = registers[Vy];
  }

  // 8xy1 - OR Vx, Vy
  void OP_8xy1() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] |= registers[Vy];
  }

  // 8xy2 - AND Vx, Vy
  void OP_8xy2() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] &= registers[Vy];
  }
  // 8xy3 - XOR Vx, Vy
  void OP_8xy3() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] ^= registers[Vy];
  }

  // 8xy4 - ADD Vx, Vy
  void OP_8xy4() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    uint16_t sum = registers[Vx] + registers[Vy];

    // set carry bit
    registers[0xF] = sum >> 8u;

    // implicitly truncate carry bit
    registers[Vx] = sum;
  }

  // 8xy5 - SUB Vx, Vy
  void OP_8xy5() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    uint16_t diff = registers[Vx] - registers[Vy];
    // if Vx > Vy = Vx - Vy > 0, then we set borrow bit
    registers[0xF] = diff > 0;
    registers[Vx] = diff;
  }

  // 8xy6 - SHR Vx {, Vy}
  void OP_8xy6() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[0xF] = registers[Vx] & 1;
    registers[Vx] >>= 1;
  }

  // 8xy7 - SUBN Vx, Vy
  void OP_8xy7() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    uint16_t diff = registers[Vy] - registers[Vx];
    // if Vy > Vx = Vy - Vx > 0, then we set borrow bit
    registers[0xF] = diff > 0;
    registers[Vx] = diff;
  }

  // 8xyE - SHL Vx {, Vy}
  void OP_8xyE() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[0xF] = (registers[Vx] >> 11) & 1;
    registers[Vx] <<= 1;
  }

  // 9xy0 - SNE Vx, Vy
  void OP_9xy0() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] != registers[Vy]) pc += 2;
  }

  // Annn - LD I, addr
  void OP_Annn() {
    uint16_t addr = opcode & 0x0FFF;

    index = addr;
  }

  // Bnnn - JP V0, addr
  void OP_Bnnn() {
    uint16_t addr = opcode & 0x0FFF;

    pc = registers[0x0] + addr;
  }

  // Cxkk - RND Vx, byte
  void OP_Cxkk() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t byte = opcode & 0x00FF;

    registers[Vx] = randByte(randGen) & byte;
  }

  // Dxyn - DRW Vx, Vy, nibble
  void OP_Dxyn() {
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
        uint32_t* video_pixel = &video[(y + dy) * VIDEO_WIDTH + (x + dx)];

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
  void OP_Ex9E() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t key = registers[Vx];
    if (keypad[Vx]) pc += 2;
  }

  // ExA1 - SKNP Vx
  void OP_ExA1() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint8_t key = registers[Vx];
    if (!keypad[Vx]) pc += 2;
  }

  // Fx07 - LD Vx, DT
  void OP_Fx07() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;

    registers[Vx] = delayTimer;
  }

  // Fx0A - LD Vx, K
  void OP_Fx0A() {
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
  void OP_Fx15() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;

    delayTimer = registers[Vx];
  }

  // Fx18 - LD ST, Vx
  void OP_Fx18() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;

    soundTimer = registers[Vx];
  }

  // Fx1E - ADD I, Vx
  void OP_Fx1E() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;

    index += registers[Vx];
  }

  // Fx29 - LD F, Vx
  void OP_Fx29() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint16_t digit = registers[Vx];

    index = FONTSET_START_ADDRESS + digit * 5;
  }
  // Fx33 - LD B, Vx
  void OP_Fx33() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;
    uint16_t value = registers[Vx];

    memory[index + 2] = value % 10;
    index /= 10;
    memory[index + 1] = value % 10;
    index /= 10;
    memory[index] = value % 10;
  }

  // Fx55 - LD [I], Vx
  void OP_Fx55() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;

    for (int v = 0; v <= Vx; v++) {
      memory[index + v] = registers[v];
    }
  }

  // Fx65 - LD Vx, [I]
  void OP_Fx65() {
    uint8_t Vx = (opcode & 0x0F00) >> 8;

    for (int v = 0; v <= Vx; v++) {
      registers[v] = memory[index + v];
    }
  }

  void Cycle() {
    // Fetch
    opcode = (memory[pc] << 8u) | memory[pc + 1];

    // Increment the PC before we execute anything
    pc += 2;

    // Decode and Execute
    ((*this).*(table[(opcode & 0xF000u) >> 12u]))();

    // Decrement the delay timer if it's been set
    if (delayTimer > 0) {
      --delayTimer;
    }

    // Decrement the sound timer if it's been set
    if (soundTimer > 0) {
      --soundTimer;
    }
  }
};

class Platform {
 public:
  Platform(char const* title, int windowWidth, int windowHeight,
           int textureWidth, int textureHeight) {
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow(title, 0, 0, windowWidth, windowHeight,
                              SDL_WINDOW_SHOWN);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                SDL_TEXTUREACCESS_STREAMING, textureWidth,
                                textureHeight);
  }

  ~Platform() {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
  }

  void Update(void const* buffer, int pitch) {
    SDL_UpdateTexture(texture, nullptr, buffer, pitch);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
  }

  bool ProcessInput(uint8_t* keys) {
    bool quit = false;

    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT: {
          quit = true;
        } break;

        case SDL_KEYDOWN: {
          switch (event.key.keysym.sym) {
            case SDLK_ESCAPE: {
              quit = true;
            } break;

            case SDLK_x: {
              keys[0] = 1;
            } break;

            case SDLK_1: {
              keys[1] = 1;
            } break;

            case SDLK_2: {
              keys[2] = 1;
            } break;

            case SDLK_3: {
              keys[3] = 1;
            } break;

            case SDLK_q: {
              keys[4] = 1;
            } break;

            case SDLK_w: {
              keys[5] = 1;
            } break;

            case SDLK_e: {
              keys[6] = 1;
            } break;

            case SDLK_a: {
              keys[7] = 1;
            } break;

            case SDLK_s: {
              keys[8] = 1;
            } break;

            case SDLK_d: {
              keys[9] = 1;
            } break;

            case SDLK_z: {
              keys[0xA] = 1;
            } break;

            case SDLK_c: {
              keys[0xB] = 1;
            } break;

            case SDLK_4: {
              keys[0xC] = 1;
            } break;

            case SDLK_r: {
              keys[0xD] = 1;
            } break;

            case SDLK_f: {
              keys[0xE] = 1;
            } break;

            case SDLK_v: {
              keys[0xF] = 1;
            } break;
          }
        } break;

        case SDL_KEYUP: {
          switch (event.key.keysym.sym) {
            case SDLK_x: {
              keys[0] = 0;
            } break;

            case SDLK_1: {
              keys[1] = 0;
            } break;

            case SDLK_2: {
              keys[2] = 0;
            } break;

            case SDLK_3: {
              keys[3] = 0;
            } break;

            case SDLK_q: {
              keys[4] = 0;
            } break;

            case SDLK_w: {
              keys[5] = 0;
            } break;

            case SDLK_e: {
              keys[6] = 0;
            } break;

            case SDLK_a: {
              keys[7] = 0;
            } break;

            case SDLK_s: {
              keys[8] = 0;
            } break;

            case SDLK_d: {
              keys[9] = 0;
            } break;

            case SDLK_z: {
              keys[0xA] = 0;
            } break;

            case SDLK_c: {
              keys[0xB] = 0;
            } break;

            case SDLK_4: {
              keys[0xC] = 0;
            } break;

            case SDLK_r: {
              keys[0xD] = 0;
            } break;

            case SDLK_f: {
              keys[0xE] = 0;
            } break;

            case SDLK_v: {
              keys[0xF] = 0;
            } break;
          }
        } break;
      }
    }

    return quit;
  }

 private:
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* texture;
};

int main(int argc, char** argv) {
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0] << " <Scale> <Delay> <ROM>\n";
    std::exit(EXIT_FAILURE);
  }

  int videoScale = std::stoi(argv[1]);
  int cycleDelay = std::stoi(argv[2]);
  const string romFilename = argv[3];

  Platform platform("CHIP-8 Emulator", VIDEO_WIDTH * videoScale,
                    VIDEO_HEIGHT * videoScale, VIDEO_WIDTH, VIDEO_HEIGHT);

  Chip8 chip8;
  chip8.LoadROM(romFilename);

  int videoPitch = sizeof(chip8.video[0]) * VIDEO_WIDTH;

  auto lastCycleTime = std::chrono::high_resolution_clock::now();
  bool quit = false;

  while (!quit) {
    quit = platform.ProcessInput(chip8.keypad);

    auto currentTime = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float, std::chrono::milliseconds::period>(
                   currentTime - lastCycleTime)
                   .count();

    if (dt > cycleDelay) {
      lastCycleTime = currentTime;

      chip8.Cycle();

      platform.Update(chip8.video, videoPitch);
    }
  }

  return 0;
}

// int main(int argc, char *argv[])
// {
//   if (argc != 2)
//   {
//     cerr << "Usage: " << argv[0] << " <rom_filename>" << endl;
//     return 1;
//   }

//   cout << "Hello, World!~" << endl;

//   for (uint8_t x : emu.registers)
//   {
//     printf("%d\n", x);
//   }

//   emu.LoadROM(argv[1]);

//   for (uint8_t x : emu.memory)
//   {
//     printf("%02x ", x);
//   }

//   return 0;
// }