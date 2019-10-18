#include "player.h"
#include "scene.h"
#include <stdio.h>
#include <string.h>


// struct interactable_list_t items;
// struct interactable_t *selected_item = NULL;

// void p_take_item(struct interactable_t *item)
// {
//     it_unlink(item);
//     it_append(&items, item);
// }

// void p_drop_item(struct interactable_t *item)
// {
//     it_unlink(item);
// }

// void p_select_item(struct interactable_t *item)
// {
//     selected_item = item;
// }

// void p_use_selected()
// {
//     // it_interact(selected_item, INTERACTABLE_FLAG_USABLE);
//     selected_item = NULL;
// }

// void p_use_selected_on_item(struct interactable_t *item)
// {

// }

void p_next_cmd()
{
    static char cmd_buffer[64];
    // struct scene_t *current_scene;
    struct interactable_t *interactable;
    struct dat_attrib_t *inspect;

    printf("--> ");
    fgets(cmd_buffer, sizeof(cmd_buffer), stdin);
    cmd_buffer[strlen(cmd_buffer) - 1] = '\0';

    if(!strcmp(cmd_buffer, "inspect"))
    {
        printf("inspect what?\n--> ");
        fgets(cmd_buffer, sizeof(cmd_buffer), stdin);
        cmd_buffer[strlen(cmd_buffer) - 1] = '\0';
        interactable = get_interactable(get_current_scene(), cmd_buffer);

        if(interactable)
        {
            inspect = dat_get_attrib(interactable->attribs, "inspect");

            if(inspect)
            {
                if(inspect->type == DAT_ATTRIB_TYPE_STRING)
                {
                    printf("%s\n", inspect->data.str_data);
                }
                else if(inspect->type == DAT_ATTRIB_TYPE_CODE)
                {
                    vm_execute_code(&inspect->data.code);
                }
            }
        }
    }   
}