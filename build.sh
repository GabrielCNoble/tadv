gcc -c main.c -Wall -o objects/main.o
gcc -c vm.c -Wall -o objects/vm.o
gcc -c scene.c -Wall -o objects/scene.o
gcc -c interactable.c -Wall -o objects/interactable.o
gcc -c player.c -Wall -o objects/player.o
gcc -c dat.c -Wall -o objects/dat.o
gcc objects/*.o -o build/main
