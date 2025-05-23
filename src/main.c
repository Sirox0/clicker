#include <vulkan/vulkan.h>

#include <stdlib.h>
#include <time.h>

#include "sdl.h"
#include "vkInit.h"
#include "vkFunctions.h"
#include "game.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    srand(time(NULL));

    sdlInit();
    vkInit();
    gameInit();

    sdlLoop();

    vkDeviceWaitIdle(vkglobals.device);
    gameQuit();
    vkQuit();
    sdlQuit();
    return 0;
}