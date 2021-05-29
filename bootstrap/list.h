#ifndef __BOTSTRAP_LIST_H__
#define __BOTSTRAP_LIST_H__

#include <stdbool.h>

struct list_node {
   struct list_node *next;
   struct list_node *prev;
};

struct list_head {
   struct list_node head;
};

void list_setup(struct list_head *head);
bool list_empty(const struct list_head *head);
void list_link_before(struct list_node *position, struct list_node *item);
void list_link_after(struct list_node *position, struct list_node *item);
void list_unlink(struct list_node *item);

#endif  // __BOOTSTRAP_LIST_H__
