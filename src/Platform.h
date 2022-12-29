//
// Created by cf12 on 12/28/22.
//

#ifndef EMULATORCHIP8_PLATFORM_H
#define EMULATORCHIP8_PLATFORM_H

#include <SDL.h>

class Platform {
public:
    Platform(char const *title, int windowWidth, int windowHeight,
             int textureWidth, int textureHeight);
    ~Platform();

    void Update(void const *buffer, int pitch);

    bool ProcessInput(uint8_t *keys);

private:
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
};


#endif //EMULATORCHIP8_PLATFORM_H