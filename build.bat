gcc -c -I"SDL/include/SDL2" main.c -o main.o
gcc -c vm.c -o vm.o
gcc -c scene.c -o scene.o
gcc -c interactable.c -o interactable.o
gcc -c player.c -o player.o
gcc -c dat.c -o dat.o
gcc vm.o dat.o interactable.o player.o scene.o main.o -L"SDL/lib" -lSDL2.dll -o main.exe