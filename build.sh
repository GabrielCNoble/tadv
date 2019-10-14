gcc -c -I"SDL/include/SDL2" main.c -o objects/main.o
gcc -c vm.c -o objects/vm.o
gcc -c scene.c -o objects/scene.o
gcc -c interactable.c -o objects/interactable.o
gcc -c player.c -o objects/player.o
gcc -c dat.c -o objects/dat.o
gcc objects/*.o -L"SDL/lib" -lSDL2 -o build/main
