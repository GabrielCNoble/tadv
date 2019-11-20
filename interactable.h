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
    INTERACTION_TYPE_SELECT,
    INTERACTION_TYPE_COMBINE,
    INTERACTION_TYPE_LAST,
};

struct interactable_t
{
    struct interactable_t *next;
    struct interactable_t *prev;
    
    // struct interactable_t *over;
    // struct interactable_t *inside;
    // struct interactable_t *under;

    struct interactable_t *children;
    struct interactable_t *parent;

    struct dat_attrib_t *attribs;

    char *name;
    uint32_t flags;
    struct code_buffer_t acions[INTERACTION_TYPE_LAST];
};

void it_load_story(char *file_name);

struct interactable_t *it_build_interactable_list(struct dat_attrib_t *attrib, struct interactable_t *parent);

struct interactable_t *it_get_interactable(struct interactable_t *start, char *name);

void it_unlink(struct interactable_t *interactable);

void it_link_before(struct interactable_t *link_to, struct interactable_t *to_link);

void it_link_after(struct interactable_t *link_to, struct interactable_t *to_link);

void it_interact(struct interactable_t *interactable, uint32_t interaction);

#endif 