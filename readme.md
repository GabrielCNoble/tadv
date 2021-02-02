#tadv

This is a small virtual machine built to create small text adventure games, similar to zork.

The assembly syntax is similar to x86, with a few extra game oriented instructions. Those instructions are mostly to get access to game objects inside scripts. Also, some instructions are used to test strings (regex instructions are something I plan to add eventually).

The virtual machine has 4 64 bit general purpose registers `(r0, r1, r2 and r3)`, 2 registers dedicated to loading interactable and attributes from interactables `(ri0 and ri1)`, and one register to hold a reference to a scene `(reg_sc)`.

Move instructions can move data directly between different memory locations, without requiring being loaded first into a register. Heh, the perks of a software cpu.

The project sort of grew out of control, becoming less of a text adventure virtual machine, to be more of a general purpose cpu. Since implementing the game logic using assembly quickly became cumbersome, the idea of creating a higher level language for that purpose appeared. The syntax is not yet fully worked out, but is also something I plan to do. 


###.dat
Scenes are defined in a format called .dat, which defines scenes, interactables inside the scene and attributes of interactables. Each scene can contain as many interactables as desired. 

Each interactable can have as many attributes as desired. And attributes can be numeric, boolean, string or a script. 


```
{
    scene = "scene0";

    description = "It's a room. You have entered it.";
    
    interactables = {
        door = {
            inspect = (
                ldsc "scene0";
                ldi ri0, "door";
                ldia r0, ri0, "is_locked";

                cmp [r0], 0;
                je _is_unlocked;
                print "Door locked!\n";
                ret 0;

                _is_unlocked:
                print "Door unlocked!\n";
            );

            walk = (
                ldsc "scene0";
                ldi ri0, "door";
                ldia r0, ri0, "is_locked";
                cmp [r0], 0;
                je _go_through;
                print "The door is locked.\n";
                ret 0;

                _go_through:
                chgsc "scene1";
            );

            is_locked = 1;
        };

        button = {
            inspect = "It's a big, red button";

            use = (
                ldsc "scene0";
                ldi ri0, "door";
                ldia r0, ri0, "is_locked";
                cmp [r0], 0;
                je _already_pressed;
                mov [r0], 0;
                print "You press the red button. You hear a loud, metallic click behind you.\n";
                ret 0;
                _already_pressed:
                print "You press the red button, but nothing happens.\n";
            );
        };

        ceiling = {
            inspect = "Ceiling";
        };

        floor = {
            inspect = (
                print "It is a \n";
                mov r0, 10;
                again:
                print "FUCKING\n";
                dec r0;
                cmp r0, 0;
                jne again;
                print "FLOOOOOOR!!!!!\n";
            );
        };

        chest = {
            inspect = (
                ldsc "scene0";
                ldi ri0, "chest";
                ldia r0, ri0, "chest_open";
                
                cmp [r0], 0;
                jne _chest_open;
                print "Chest locked :(\n";
                ret 0;

                _chest_open:
                print "Chest unlocked :)\n";
            );

            open = (
                ldsc "scene0";
                ldi ri0, "chest";
                ldia r0, ri0, "chest_open";
                mov [r0], 1;
            );

            chest_open = 0;

            inside = {
                key = {
                    inspect = "Key";
                };

                candle = {
                    inspect = "Candle";
                };

                gem = {
                    inspect = "Gem";
                };
            };
        };
    };
}
{
    scene = "scene1";

    description = "This is another room. It's dark and its air is murky. The only source of light is a crack in the ceiling. Looking up you realize how tall the ceiling is. A loud noise startles you, making you turn around to the door you just walked through. For some reason the doorway caved in. You're trapped.\n";

    interactables = {
        door = {
            walk = "You dart your eyes around the caved in passage, but you find no way to get through it.\n"; 
        };
    };
}
```


This is the contents of `story.dat`. 

Scripts are blocks of assembly code that test and modify attributes of interactables, and they define behaviours for interactables. This behaviour is essentially the game logic.

Each interactable has some "reserved" attributes that can be defined. Those attributes are the actions that can be executed on the interactables. Things like "open", "inspect", "use", etc. During gameplay, if one of those commands is entered in the console, the game will look for and execute whatever is defined for those attributes.

To simplify things for simple actions when those attributes are called (like simply printing a description of something upon inspection), those reserved attributes may be defined as strings instead of blocks of code. Then, the game simply prints them. 

Those scripts may simply print something to the console, maybe depending on the value of some attribute of that interactable, or the value of an attribute of another interactable. The scripts may also change values of attributes, or cause scene changes. There's no special interactable, like doors, that trigger a scene change. Anything can cause that at any time. So, for example, maybe an enchanted (or cursed) note, forgotten over an old desk, may cause the protagonist to be teleported to a parallel dimension upon inspected. Who knows?

Taking the chest interactable, for example.

```
chest = {
    inspect = (
        ldsc "scene0";
        ldi ri0, "chest";
        ldia r0, ri0, "chest_open";
        
        cmp [r0], 0;
        jne _chest_open;
        print "Chest locked :(\n";
        ret 0;

        _chest_open:
        print "Chest unlocked :)\n";
    );

    open = (
        ldsc "scene0";
        ldi ri0, "chest";
        ldia r0, ri0, "chest_open";
        mov [r0], 1;
    );

    chest_open = 0;

    inside = {
        key = {
            inspect = "Key";
        };

        candle = {
            inspect = "Candle";
        };

        gem = {
            inspect = "Gem";
        };
    };
};
```

This interactable defines three reserved attributes: `inspect`, `open` and `inside`. `inside` is a special attribute because it allows the game to present a list of things that are contained inside another. For example, items inside a chest.

It also defines the attribute `chest_open`, which is just a generic attribute. In this case, it holds whether the chest is unlocked or not.

`inspect` defines what's supposed to happen when the player inspects the chest. First, the reference of "scene0" is loaded into `reg_sc`, then the reference of an interactable in that scene is obtained and loaded into `ri0`, and then the value of the generic attribute `chest_open` is loaded into the general purpose register `r0`. The value is then tested, and depending on the value, it prints whether the chest is open or closed.

`open` defines a simple script where it just sets the value of `chest_open` to 1.
