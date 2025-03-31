/** @file libpriqueue.c
 */

#include <stdlib.h>
#include <stdio.h>

#include "libpriqueue.h"

// Node helper methods

node_t *new_node (void *item, node_t *next) {
  node_t * node = malloc(sizeof(node_t));
  node->item = item;
  node->next = next;
  return node;
}

void *destroy_node (node_t *node) {
  if (node == NULL) return NULL;

  void *item = node->item;
  free(node);
  return item;
}

void destroy_list (node_t *node) {
  if (node == NULL) return;

  destroy_list(node->next);
  free(node);
}

node_t *node_at (node_t *node, int index) {
  if (index < 0 || node == NULL) return NULL;

  return index == 0 ? node : node_at(node->next, index - 1);
}

int list_size(node_t *node) {
  return node == NULL ? 0 : 1 + list_size(node->next);
}


/**
  Initializes the priqueue_t data structure.
  
  Assumtions
    - You may assume this function will only be called once per instance of priqueue_t
    - You may assume this function will be the first function called using an instance of priqueue_t.
  @param q a pointer to an instance of the priqueue_t data structure
  @param comparer a function pointer that compares two elements. If comparer(x, y) < 0, then the priority of x is higher than the priority of y and therefore should be placed earlier in the queue.
  See also @ref comparer-page
 */
void priqueue_init(priqueue_t *q, int(*comparer)(const void *, const void *))
{
  q->comparer = comparer;
  q->top = NULL;
}


/**
  Insert the specified element into this priority queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr a pointer to the data to be inserted into the priority queue
  @return The zero-based index where ptr is stored in the priority queue, where 0 indicates that ptr was stored at the front of the priority queue.
 */
int priqueue_offer(priqueue_t *q, void *ptr)
{
  // special case: if queue is empty, just insert the value at top
  if (q->top == NULL) {
    node_t *node = new_node(ptr, NULL);
    q->top = node;
    return 0;
  }

  // otherwise, iterate over the queue list
  int i = 0;
  node_t *prev = NULL;
  node_t *target = q->top;

  for (; target != NULL; ++i) {
    if(q->comparer(ptr, target->item) < 0) {
      node_t *node = new_node(ptr, target);

      // if we are placing at start of list, point q->top to this new node
      if(i == 0) {
        q->top = node;
      }
      // otherwise, point prev->next to the new node
      else {
        prev->next = node;
      }
      return i;
    }
    prev = target;
    target = target->next;
  }

  // if wasn't placed, place item at end
  node_t *node = new_node(ptr, target);
  prev->next = node;
	return i;
}


/**
  Retrieves, but does not remove, the head of this queue, returning NULL if
  this queue is empty.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return pointer to element at the head of the queue
  @return NULL if the queue is empty
 */
void *priqueue_peek(priqueue_t *q)
{
	return priqueue_at(q, 0);
}


/**
  Retrieves and removes the head of this queue, or NULL if this queue
  is empty.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return the head of this queue
  @return NULL if this queue is empty
 */
void *priqueue_poll(priqueue_t *q)
{
  return priqueue_remove_at(q, 0);
}




/**
  Returns the element at the specified position in this list, or NULL if
  the queue does not contain an index'th element.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of retrieved element
  @return the index'th element in the queue
  @return NULL if the queue does not contain the index'th element
 */
void *priqueue_at(priqueue_t *q, int index)
{
	node_t * node = q->top == NULL ? NULL : node_at(q->top, index);
  return node == NULL ? NULL : node->item;
}


/**
  Removes all instances of ptr from the queue. 
  
  This function should not use the comparer function, but check if the data contained in each element of the queue is equal (==) to ptr.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr address of element to be removed
  @return the number of entries removed
 */
int priqueue_remove(priqueue_t *q, void *ptr)
{
  
  int removed = 0;
  node_t *target = q->top;
  node_t *prev = NULL;

  while (target != NULL) {
    if(target->item == ptr) { // TODO: compare by pointer or value?
      // special case: if target == q->top, must re-assign q->top to next element
      if (target == q->top) q->top = q->top->next;

      if (prev != NULL) prev->next = target->next;
      node_t *remove = target;
      target = target->next;
      destroy_node(remove);
      removed++;
    }
    else {
      prev = target;
      target = target->next;
    }
  }

  return removed;
}


/**
  Removes the specified index from the queue, moving later elements up
  a spot in the queue to fill the gap.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of element to be removed
  @return the element removed from the queue
  @return NULL if the specified index does not exist
 */
void *priqueue_remove_at(priqueue_t *q, int index)
{
  if(index < 0 || q->top == NULL) return NULL;
  else if (index == 0) {
    node_t *remove = q->top;
    q->top = q->top->next;
    return destroy_node(remove);
  }
  else { // index > 0
    node_t *prev = node_at(q->top, index - 1);
    if(prev == NULL) return NULL;

    node_t *remove = prev->next;
    if(remove == NULL) return NULL;

    prev->next = remove->next;
    return destroy_node(remove);
  }
}


/**
  Return the number of elements in the queue.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return the number of elements in the queue
 */
int priqueue_size(priqueue_t *q)
{
	return list_size(q->top);
}


/**
  Destroys and frees all the memory associated with q.
  
  @param q a pointer to an instance of the priqueue_t data structure
 */
void priqueue_destroy(priqueue_t *q)
{
  destroy_list(q->top);
}
