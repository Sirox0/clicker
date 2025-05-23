#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <stdio.h>
#include <stdlib.h>

#include "numtypes.h"
#include "config.h"
#include "game.h"

SDL_Window* window;
u8 loopActive = 1;
u32 deltaTime = 0.0f;

void sdlInit() {
    SDL_Init(SDL_INIT_EVENTS);
    window = SDL_CreateWindow(TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);

    if (window == NULL) {
        printf("failed to create window\n");
        exit(1);
    }
}

u32 sdlGetTicks() {
    return SDL_GetTicks();
}

SDL_MouseButtonFlags sdlQueryMouseState(f32* pX, f32* pY) {
    return SDL_GetMouseState(pX, pY);
}

void sdlQueryInstanceExtensions(u32* pExtCount, const char*** pExts) {
    *pExts = (const char**)SDL_Vulkan_GetInstanceExtensions(pExtCount);
}

void sdlCreateSurface(VkInstance instance, VkSurfaceKHR* pSurface) {
    if (SDL_Vulkan_CreateSurface(window, instance, NULL, pSurface) != true) {
        printf("failed to create vulkan surface for sdl window\n");
        exit(1);
    }
}

void sdlDestroySurface(VkInstance instance, VkSurfaceKHR surface) {
    SDL_Vulkan_DestroySurface(instance, surface, NULL);
}

void sdlQueryWindowResolution(u32* pW, u32* pH) {
    // width and height normally are positive integers
    if (SDL_GetWindowSizeInPixels(window, (i32*)pW, (i32*)pH) != true) {
        printf("failed to get window size from sdl\n");
        exit(1);
    }
}

void sdlLoop() {
    SDL_Event event;
    while (loopActive) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                loopActive = 0;
                break;
            }
        }

        u32 startTime = sdlGetTicks();

        gameRender();

        deltaTime = sdlGetTicks() - startTime;
    }
}

void sdlQuit() {
    SDL_DestroyWindow(window);
    SDL_Quit();
}