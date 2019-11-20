#include "interactable.h"
#include <stdio.h>

struct interactable_t *it_interactables = NULL;

void it_load_story(char *file_name)
{
    FILE *file;
    char *file_buffer;
    long file_len;

    // file = fopen("/LovecraftScenes/intro.scene", "rb");
    file = fopen("/data/story.dat", "rb");
    
    if(file == NULL)
    {
        return -1;
    }

    fseek(file, 0, SEEK_END);
    file_len = ftell(file);
    rewind(file);
    file_buffer = calloc(1, file_len + 1);
    fread(file_buffer, file_len, 1, file);
    fclose(file);
    file_buffer[file_len] = '\0';

    struct dat_attrib_t *attribs;
    struct dat_attrib_t *attrib;
    struct dat_attrib_t *interactable_name;
    struct interactable_t *interactable;

    attribs = dat_parse_dat_string(file_buffer);

    it_interactables = it_build_interactable_list(attribs, NULL);
    // attrib = attribs;
    // while(attrib)
    // {
    //     interactable_name = dat_get_attrib(attrib->data.attrib, "scene");

    //     if(interactable_name && interactable_name->type == DAT_ATTRIB_TYPE_STRING)
    //     {
    //         interactable = calloc(1, sizeof(struct interactable_t));
    //         interactable->
    //         // scene = calloc(1, sizeof(struct scene_t));
    //         // scene->attribs = attrib->data.attrib;
    //         // scene->name = scene_attrib->data.str_data;

    //         // build_interactable_list(scene);

    //         // scene->next = scenes;
    //         // scenes = scene;
    //     }

    //     attrib = attrib->next;
    // }
    return 0;
}

struct interactable_t *it_build_interactable_list(struct dat_attrib_t *attrib, struct interactable_t *parent)
{
    struct dat_attrib_t *name;
    struct interactable_t *interactable;
    struct interactable_t *interactables = NULL;
    struct dat_attrib_t *children;

    while(attrib)
    {
        name = dat_get_attrib(attrib->data.attrib, "name");
        
        if(name && name->type == DAT_ATTRIB_TYPE_STRING)
        {
            interactable = calloc(1, sizeof(struct interactable_t));
            interactable->name = name->data.str_data; 
            interactable->parent = parent;

            children = dat_get_attrib(attrib->data.attrib, "interactables");

            if(children && children->type == DAT_ATTRIB_TYPE_STRUCT)
            {
                interactable->children = it_build_interactable_list(children->data.attrib, interactable);
            }

            interactable->next = interactables;

            if(interactable->next)
            {
                interactable->next->prev = interactable;
            }

            interactables = interactable;
        }
    }

    return interactables;
}

struct interactable_t *it_get_interactable_recursive(struct interactable_t *start, struct vm_lexer_t *lexer)
{
    if(lexer->token.token_class == TOKEN_CLASS_STRING_CONSTANT)
    {
        while(start)
        {
            if(!strcmp(start->name, lexer->token.constant.ptr_constant))
            {
                vm_lex_one_token(lexer);

                if(lexer->token.token_class == TOKEN_CLASS_PUNCTUATOR &&
                   lexer->token.token_type == TOKEN_PUNCTUATOR_DOT)
                {
                    return it_get_interactable_recursive(start->children, lexer);
                }
                
                return start;
            }

            start = start->next;
        }
    }

    return NULL;
}
struct interactable_t *it_get_interactable(struct interactable_t *start, char *name)
{
    struct vm_lexer_t lexer;
    vm_init_lexer(&lexer, name);
    vm_lex_one_token(&lexer);
    
    /* avoiding exposing it_interactables to other files. For a global
    search, just pass NULL as start point */
    start = start ? start : it_interactables;
    return it_get_interactable_recursive(start, &lexer); 
}

void it_unlink(struct interactable_t *interactable)
{
    if(interactable)
    {
        if(interactable->prev)
        {
            interactable->prev->next = interactable->next;
        }

        if(interactable->next)
        {
            interactable->next->prev = interactable->prev;
        }
    
        interactable->next = NULL;
        interactable->prev = NULL;
    }
}

void it_link_before(struct interactable_t *link_to, struct interactable_t *to_link)
{
    if(link_to && to_link)
    {
        if(link_to->prev)
        {
            link_to->prev->next = to_link;
        }

        to_link->prev = link_to->prev;
        to_link->next = link_to;
        link_to->prev = to_link;
    }
}

void it_link_after(struct interactable_t *link_to, struct interactable_t *to_link)
{
    if(link_to && to_link)
    {
        if(link_to->next)
        {
            link_to->next->prev = to_link;
        }

        to_link->next = link_to->next;
        to_link->prev = link_to;
        link_to->next = to_link;
    }
}

void it_interact(struct interactable_t *interactable, uint32_t interaction)
{
    if(interactable)
    {
        if(interactable->flags & interaction)
        {
            
        }
    }
}