#ifndef INTERACTABLE_H
#define INTERACTABLE_H
#include "vm.h"
#include "dat.h"

enum INTERACTION_TYPE
{
    INTERACTION_TYPE_INSPECT = 0,
    INTERACTION_TYPE_TAKE,
    INTERACTION_TYPE_DROP,
    INTERACTION_TYPE_WALK,
    INTERACTION_TYPE_OPEN,
    INTERACTION_TYPE_CLOSE,
    INTERACTION_TYPE_EQUIP,
    INTERACTION_TYPE_UNEQUIP,
    INTERACTION_TYPE_USE,
    INTERACTION_TYPE_USE_ON,
    INTERACTION_TYPE_COMBINE,
};

struct interactable_t
{
    struct interactable_t *next;
    struct interactable_t *prev;
    struct interactable_list_t *list;
    struct dat_attrib_t *attribs;
    char *name;
    uint32_t flags;
    // struct code_buffer_t code;
};

struct interactable_list_t
{
    struct interactable_t *interactables;
    struct interactable_t *last_interactable;
};

void it_unlink(struct interactable_t *interactable);

void it_link_before(struct interactable_t *link_to, struct interactable_t *to_link);

void it_link_after(struct interactable_t *link_to, struct interactable_t *to_link);

void it_prepend(struct interactable_list_t *list, struct interactable_t *interactable);

void it_append(struct interactable_list_t *list, struct interactable_t *interactable);

void it_interact(struct interactable_t *interactable, uint32_t interaction);

#endif 