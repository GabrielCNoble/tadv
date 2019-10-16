gcc -c main.c -Wall -o main.o
gcc -c vm.c -Wall -o vm.o
gcc -c scene.c -Wall -o scene.o
gcc -c interactable.c -Wall -o interactable.o
gcc -c player.c -Wall -o player.o
gcc -c dat.c -Wall -o dat.o
gcc vm.o dat.o interactable.o player.o scene.o main.o -o main.exe