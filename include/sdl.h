#ifndef SDL_H
#define SDL_H

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

#include "numtypes.h"

extern SDL_Window* window;
extern u8 loopActive;
extern u32 deltaTime;

void sdlInit();
u32 sdlGetTicks();
SDL_MouseButtonFlags sdlQueryMouseState(f32* pX, f32* pY);
void sdlQueryInstanceExtensions(u32* pExtCount, const char*** pExts);
void sdlCreateSurface(VkInstance instance, VkSurfaceKHR* pSurface);
void sdlDestroySurface(VkInstance instance, VkSurfaceKHR surface);
void sdlQueryWindowResolution(u32* pW, u32* pH);
void sdlLoop();
void sdlQuit();

#endif