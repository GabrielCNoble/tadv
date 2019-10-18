#include "interactable.h"
#include <stdio.h>

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