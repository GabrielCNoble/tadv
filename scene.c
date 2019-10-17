#include "scene.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct scene_t *scenes = NULL;

void load_scenes()
{
    FILE *file;
    char *file_buffer;
    // char *identifier;
    long file_len;

    struct scene_t *scene;

    file = fopen("test.dat", "r");
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
    // struct dat_attrib_t *intro;


    // struct token_t *tokens = vm_lex_code(file_buffer);
    // vm_print_tokens(tokens);
    // return;


    attribs = dat_parse_dat_string(file_buffer);    
    // attrib = attribs;

    attrib = attribs->data.attrib;
    while(attrib)
    {
        printf("%s = ", attrib->name);

        switch(attrib->type)
        {
            case DAT_ATTRIB_TYPE_STRING:
                printf("\"%s\"", attrib->data.str_data);
            break;

            case DAT_ATTRIB_TYPE_INT:
                printf("%d", attrib->data.int_data);
            break;
            
            case DAT_ATTRIB_TYPE_FLOAT:
                printf("%f", attrib->data.flt_data);
            break;

            case DAT_ATTRIB_TYPE_STRUCT:
                printf("{}");
            break;

            case DAT_ATTRIB_TYPE_CODE:
                printf("()");
            break;
        }

        printf("\n");

        attrib = attrib->next;
    }
    // attrib = dat_get_attrib(attribs->data.attrib, "attrib");

    // if(attrib)
    // {
    //     // printf("has attrib!\n");
    //     // printf("%s\n", attrib->data.str_data);
    // }

    return;


    while(attrib)
    {
        scene = calloc(1, sizeof(struct scene_t));
        scene->attribs = attrib->data.attrib;
        scene_attrib = dat_get_attrib(scene->attribs, "scene");
        scene->name = scene_attrib->data.str_data;

        scene->next = scenes;
        scenes = scene;

        attrib = attrib->next;
    }

    scene = scenes;

    while(scene)
    {
        scene_attrib = dat_get_attrib(scene->attribs, "logic");

        if(scene_attrib->type == DAT_ATTRIB_TYPE_CODE)
        {
            // if(vm_assemble_code(&scene->code, scene_attrib->data.str_data))
            // {
            //     printf("%s\n", vm_get_last_error());
            // }
            // else
            // {
                printf("return value: %I64d\n", vm_execute_code(&scene_attrib->data.code));
                vm_print_registers();
            // }
        }
        else
        {
            printf("welp, no code to run...\n");
        }
        scene = scene->next;
    }
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