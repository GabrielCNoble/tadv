#ifndef PLAYER_H
#define PLAYER_H

#include "interactable.h"

void p_take_item(struct interactable_t *item);

void p_drop_item(struct interactable_t *item);

void p_select_item(struct interactable_t *item);

void p_use_selected();

void p_use_selected_on_item(struct interactable_t *item);

void p_next_cmd();


#endif