/** @file libpriqueue.h
 */

#ifndef LIBPRIQUEUE_H_
#define LIBPRIQUEUE_H_

/**
 * Node Data Structure (for representing nodes in the priority queue)
*/
typedef struct _node_t
{
  void *item; // pointer to the item referred to by this node
  struct _node_t *next; // pointer to the next node (or NULL)
} node_t;

/**
  Priqueue Data Structure
*/
typedef struct _priqueue_t
{
  int (*comparer)(const void *, const void *);
  node_t *top;
} priqueue_t;

/*
  For a given priqueue_t<T> (elements of type T)...

  comparer: (l: T, r: T) -> int
    - given 2 elements of type T, returns an int representing the comparison status of the element priority values
    - comparer(l, r) < 0 <=> l is higher priority than r
    - comparer(l, r) > 0 <=> l is lower priority than r
    - comparer(l, r) == 0 <=> l is same priority as r
*/

// node helper methods
node_t *new_node (void *item, node_t *next); // mallocs & returns new node with fields filled
void *destroy_node (node_t *node); // frees node & returns item pointer
void destroy_list (node_t *node); // recursively frees node list
node_t *node_at (node_t *node, int index); // gets the node at the given index in the node chain
int list_size(node_t *node);

// priqueue methods
void   priqueue_init     (priqueue_t *q, int(*comparer)(const void *, const void *));

int    priqueue_offer    (priqueue_t *q, void *ptr);
void * priqueue_peek     (priqueue_t *q);
void * priqueue_poll     (priqueue_t *q);
void * priqueue_at       (priqueue_t *q, int index);
int    priqueue_remove   (priqueue_t *q, void *ptr);
void * priqueue_remove_at(priqueue_t *q, int index);
int    priqueue_size     (priqueue_t *q);

void   priqueue_destroy  (priqueue_t *q);

#endif /* LIBPQUEUE_H_ */
