#include <iostream>

#include "Chip8.h"
#include "Platform.h"

using namespace std;

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
  chip8.load_rom(romFilename);

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

      chip8.cycle();

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

//   emu.load_rom(argv[1]);

//   for (uint8_t x : emu.memory)
//   {
//     printf("%02x ", x);
//   }

//   return 0;
// }