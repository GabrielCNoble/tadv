#include <stdio.h>

// #define SDL_MAIN_HANDLED
// #include "SDL.h"
#include "vm.h"
#include "scene.h"

int main(int argc, char *argv[])
{
    // if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
    // {
    //     printf("SDL couldn't be initialized!\n %s\n", SDL_GetError());
    //     return -1;
    // }
  
    vm_init();
    load_scenes();
}