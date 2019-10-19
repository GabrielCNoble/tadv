#include "scene.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct scene_t *scenes = NULL;
struct scene_t *current_scene = NULL;

int32_t load_scenes()
{
    FILE *file;
    char *file_buffer;
    // char *identifier;
    long file_len;

    struct scene_t *scene;

    file = fopen("story.dat", "r");
    if(file == NULL)
    {
        return -1;
    }
    fseek(file, 0, SEEK_END);
    file_len = ftell(file);
    rewind(file);
    file_buffer = calloc(1, file_len + 1);
    fread(file_buffer, 1, file_len, file);
    fclose(file);
    file_buffer[file_len] = '\0';

    struct dat_attrib_t *attribs;
    struct dat_attrib_t *attrib;
    struct dat_attrib_t *scene_attrib;

    attribs = dat_parse_dat_string(file_buffer);    

    attrib = attribs;
    while(attrib)
    {
        scene_attrib = dat_get_attrib(attrib->data.attrib, "scene");

        if(scene_attrib && scene_attrib->type == DAT_ATTRIB_TYPE_STRING)
        {
            scene = calloc(1, sizeof(struct scene_t));
            scene->attribs = attrib->data.attrib;
            scene->name = scene_attrib->data.str_data;

            build_interactable_list(scene);
            
            scene->next = scenes;
            scenes = scene;
        }

        attrib = attrib->next;
    }
    return 0;
}

struct interactable_t *interactable_list_recursive(struct dat_attrib_t *attrib)
{
    struct interactable_t *interactables = NULL;
    struct interactable_t *interactable = NULL;
    struct dat_attrib_t *interactable_attrib;

    while(attrib)
    {
        if(attrib->type == DAT_ATTRIB_TYPE_STRUCT)
        {
            interactable = calloc(1, sizeof(struct interactable_t));
            interactable->name = attrib->name;
            interactable->next = interactables;
            interactable->attribs = attrib->data.attrib;
            interactables = interactable;

            interactable_attrib = dat_get_attrib(attrib->data.attrib, "over");
            if(interactable_attrib)
            {
                interactable->over = interactable_list_recursive(interactable_attrib->data.attrib);
            }

            interactable_attrib = dat_get_attrib(attrib->data.attrib, "inside");
            if(interactable_attrib)
            {
                interactable->inside = interactable_list_recursive(interactable_attrib->data.attrib);
            }

            interactable_attrib = dat_get_attrib(attrib->data.attrib, "under");
            if(interactable_attrib)
            {
                interactable->under = interactable_list_recursive(interactable_attrib->data.attrib);
            }
        }
        attrib = attrib->next;
    }

    return interactables;
}

void build_interactable_list(struct scene_t *scene)
{
    struct dat_attrib_t *interactables_attrib;

    interactables_attrib = dat_get_attrib(scene->attribs, "interactables");

    if(interactables_attrib && interactables_attrib->type == DAT_ATTRIB_TYPE_STRUCT)
    {
        scene->interactables = interactable_list_recursive(interactables_attrib->data.attrib);
    }
}

struct interactable_t *get_interactable(struct scene_t *scene, char *name)
{
    struct interactable_t *interactable = scene->interactables;

    while(interactable)
    {
        if(!strcmp(interactable->name, name))
        {
            break;
        }

        interactable = interactable->next;
    }
    
    return interactable;
}

struct scene_t *get_scene(char *name)
{
    struct scene_t *scene = scenes;

    while(scene)
    {
        if(!strcmp(scene->name, name))
        {
            break;
        }
        scene = scene->next;
    }

    return scene;
}

void set_scene(struct scene_t *scene)
{
    current_scene = scene;
}
struct scene_t *get_current_scene()
{
    return current_scene;
}