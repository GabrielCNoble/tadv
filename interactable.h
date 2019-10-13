#ifndef INTERACTABLE_H
#define INTERACTABLE_H
#include "vm.h"


enum INTERACTABLE_FLAGS
{
    INTERACTABLE_FLAG_TAKEABLE = 1,
    INTERACTABLE_FLAG_DROPABLE = 1 << 1,
    INTERACTABLE_FLAG_PUTABLE = 1 << 2,
    INTERACTABLE_FLAG_PRESSABLE = 1 << 3,
    INTERACTABLE_FLAG_USABLE = 1 << 4,
    INTERACTABLE_FLAG_LOOKABLE = 1 << 5,
};

struct interactable_t
{
    struct interactable_t *next;
    struct interactable_t *prev;
    struct interactable_list_t *list;
    char *name;
    uint32_t flags;
    struct code_buffer_t code;
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