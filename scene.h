#ifndef SCENE_H
#define SCENE_H

#include "vm.h"
#include "dat.h"
#include "interactable.h"

struct scene_t
{
    struct scene_t *next;
    char *name;
    struct dat_attrib_t *attribs;
    struct code_buffer_t code;
};

void load_scenes();

struct scene_t *get_scene(char *name);

#endif