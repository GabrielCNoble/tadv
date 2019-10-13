#include "interactable.h"
#include <stdio.h>

void it_unlink(struct interactable_t *interactable)
{
    if(interactable && interactable->list)
    {
        if(interactable->prev)
        {
            interactable->prev->next = interactable->next;
        }
        else
        {
            interactable->list->interactables = interactable->next;
        }

        if(interactable->next)
        {
            interactable->next->prev = interactable->prev;
        }
        else
        {
            interactable->list->last_interactable = interactable->prev;
        }

        interactable->next = NULL;
        interactable->prev = NULL;
        interactable->list = NULL;
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
        else
        {
            link_to->list->interactables = to_link;
        }

        to_link->prev = link_to->prev;
        to_link->list = link_to->list;
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
        else
        {
            link_to->list->last_interactable = to_link;
        }

        to_link->next = link_to->next;
        to_link->list = link_to->list;
        link_to->next = to_link;
    }
}

void it_prepend(struct interactable_list_t *list, struct interactable_t *interactable)
{
    if(list && interactable)
    {
        if(list == interactable->list)
        {
            return;
        }

        if(!list->interactables)
        {
            list->interactables = interactable;
            list->last_interactable = interactable;
        }
        else
        {
            it_link_before(list->interactables, interactable);
        }
    }
}

void it_append(struct interactable_list_t *list, struct interactable_t *interactable)
{
    if(list && interactable)
    {
        if(list == interactable->list)
        {
            return;
        }

        if(!list->interactables)
        {
            list->interactables = interactable;
            list->last_interactable = interactable;
        }
        else
        {
            it_link_after(list->last_interactable, interactable);
        }
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