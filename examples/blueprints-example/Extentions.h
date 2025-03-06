#pragma once
#define SDL_MAIN_HANDLED

#include <SDL.h>
#include "Program.h"
#include "Nodes.h"


SDL_Window* makeSDLWindow(int width = 640, int height = 480)
{
    std::cout << "Loading SDL\n";
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        __debugbreak(); //failed to initialise SDL D-:
    }

    auto window = SDL_CreateWindow("Output Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    //SDL_SetWindowAlwaysOnTop(window, SDL_TRUE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    if (SDL_GL_CreateContext(window) == NULL)
    {
        auto err = SDL_GetError();
        std::cout << "SDL Error : " << err;
        __debugbreak();
    }
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, SDL_TRUE);
    SDL_GL_SetSwapInterval(0);
    return window;
}

