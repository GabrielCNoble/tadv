#include "player.h"
#include "scene.h"
#include "vm.h"
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

enum PLAYER_CMDS
{
    PLAYER_CMD_INSPECT = 0,
    PLAYER_CMD_OPEN,
    PLAYER_CMD_TAKE,
    PLAYER_CMD_WALK,
    PLAYER_CMD_USE,
    PLAYER_CMD_UNKNOWN,
};

char *p_in()
{
    static char cmd_buffer[512];
    printf("--> ");
    fgets(cmd_buffer, sizeof(cmd_buffer), stdin);
    return cmd_buffer;
    // cmd_buffer[strlen(cmd_buffer) - 1] = '\0';
    // return vm_lex_code(cmd_buffer);
}

void p_next_cmd()
{
    // struct scene_t *current_scene;
    struct interactable_t *interactable;
    struct dat_attrib_t *attrib;
    struct vm_lexer_t lexer;
    uint32_t command = PLAYER_CMD_UNKNOWN;

    char *cmd_question;
    char *attrib_name;

    vm_init_lexer(&lexer, p_in());
    vm_lex_one_token(&lexer);
    // printf("after lexing\n");

    if(lexer.token.token_class == TOKEN_CLASS_IDENTIFIER)
    {
        if(!strcmp(lexer.token.constant.ptr_constant, "inspect"))
        {
            command = PLAYER_CMD_INSPECT;
            cmd_question = "Inspect what?";
        }
        else if(!strcmp(lexer.token.constant.ptr_constant, "open"))
        {
            command = PLAYER_CMD_OPEN;
            cmd_question = "Open what?";
        }
        else if(!strcmp(lexer.token.constant.ptr_constant, "take"))
        {
            command = PLAYER_CMD_TAKE;
            cmd_question = "Take what?";
        }
        else if(!strcmp(lexer.token.constant.ptr_constant, "walk"))
        {
            command = PLAYER_CMD_WALK;
            cmd_question = "Walk through?";
        }
        else if(!strcmp(lexer.token.constant.ptr_constant, "use"))
        {
            command = PLAYER_CMD_USE;
            cmd_question = "Use what?";
        }

        // printf("lexed\n");

        if(command != PLAYER_CMD_UNKNOWN)
        {
            vm_lex_one_token(&lexer);

            if(lexer.token.token_class == TOKEN_CLASS_UNKNOWN)
            {
                printf("%s\n", cmd_question);
                vm_init_lexer(&lexer, p_in());
                vm_lex_one_token(&lexer);
            }

            if(lexer.token.token_class == TOKEN_CLASS_IDENTIFIER)
            {
                interactable = get_interactable(get_current_scene(), lexer.token.constant.ptr_constant);

                switch(command)
                {
                    case PLAYER_CMD_INSPECT:
                        attrib_name = "inspect";
                    break;

                    case PLAYER_CMD_OPEN:
                        attrib_name = "open";
                    break;

                    case PLAYER_CMD_TAKE:
                        attrib_name = "take";
                    break;

                    case PLAYER_CMD_WALK:
                        attrib_name = "walk";
                    break;

                    case PLAYER_CMD_USE:
                        attrib_name = "use";
                    break;
                }

                if(interactable)
                {
                    attrib = dat_get_attrib(interactable->attribs, attrib_name);
        
                    if(attrib)
                    {
                        if(attrib->type == DAT_ATTRIB_TYPE_STRING)
                        {
                            printf("%s\n", attrib->data.str_data);
                        }
                        else if(attrib->type == DAT_ATTRIB_TYPE_CODE)
                        {
                            vm_execute_code(&attrib->data.code);
                        }
                    }
                }
            }
        }
    }
}