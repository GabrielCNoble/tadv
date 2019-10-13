#include "player.h"
#include <stdio.h>


struct interactable_list_t items;
struct interactable_t *selected_item = NULL;

void p_take_item(struct interactable_t *item)
{
    it_unlink(item);
    it_append(&items, item);
}

void p_drop_item(struct interactable_t *item)
{
    it_unlink(item);
}

void p_select_item(struct interactable_t *item)
{
    selected_item = item;
}

void p_use_selected()
{
    it_interact(selected_item, INTERACTABLE_FLAG_USABLE);
    selected_item = NULL;
}

void p_use_selected_on_item(struct interactable_t *item)
{

}