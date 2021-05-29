#include "list.h"

void list_setup(struct list_head *head)
{
    head->head.next = &head->head;
    head->head.prev = &head->head;
}

bool list_empty(const struct list_head *head)
{
    return head->head.next == &head->head;
}

void list_link_before(struct list_node *position, struct list_node *item)
{
    struct list_node *next = position;
    struct list_node *prev = position->prev;

    item->prev = prev;
    item->next = next;
    prev->next = item;
    next->prev = item;
}

void list_link_after(struct list_node *position, struct list_node *item)
{
    struct list_node *prev = position;
    struct list_node *next = position->next;

    item->prev = prev;
    item->next = next;
    prev->next = item;
    next->prev = item;
}

void list_unlink(struct list_node *item)
{
    struct list_node *prev = item->prev;
    struct list_node *next = item->next;

    prev->next = next;
    next->prev = prev;
}
