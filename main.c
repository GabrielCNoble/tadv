#include <stdio.h>

// #define SDL_MAIN_HANDLED
// #include "SDL.h"
#include "vm.h"
#include "interactable.h"
// #include "scene.h"
// #include "player.h"

int main(int argc, char *argv[])
{
    // if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
    // {
    //     printf("SDL couldn't be initialized!\n %s\n", SDL_GetError());
    //     return -1;
    // }
  
    vm_init();
    if(load_scenes() == -1)
    {
        printf("Falha ao carregar cenas\n");
        return 0;
    }

    set_scene(get_scene("intro"));

    while(1)
    {
        p_next_cmd();
    }
}