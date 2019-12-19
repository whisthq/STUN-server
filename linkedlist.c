/*
 * This file contains the implementation of a simple generic doubly-linked list
 * used by the hole punching server to keep track of incoming per requests for
 * pairing with a VM.
 *
 * Hole Punching Server version: 1.0
 *
 * Last modification: 12/18/2019
 *
 * By: Philippe Noël
 *
 * Copyright Fractal Computers, Inc. 2019
**/

#include <stdlib.h>
#include <stdio.h>

#include "linkedlist.h" // header file for this file

// initialize a new linked list
// returns a pointer to the new list
gll_t *gll_init() {
  gll_t *list = (gll_t *) malloc(sizeof(gll_t));
  list->size = 0;
  list->first = NULL;
  list->last = NULL;
  return list;
}

// initialize a new node
// takes in a pointer to the data
// returns a pointer to the new node
gll_node_t *gll_init_node(void *data) {
  gll_node_t *node = (gll_node_t *) malloc(sizeof(gll_node_t));
  node->data = data;
  node->prev = NULL;
  node->next = NULL;
  return node;
}

// find node at a given position
// takes in a pointer to a list and a position
// returns a pointer to the node or NULL on failure
gll_node_t *gll_find_node(gll_t *list, int pos) {
  if (pos > list->size)
    return NULL;

  gll_node_t *currNode;
  int currPos;
  int reverse;

  // decide where to start iterating from (font or back of the list)
  if (pos > ((list->size-1) / 2)) {
    reverse  = 1;
    currPos  = list->size - 1;
    currNode = list->last;
  } else {
    reverse  = 0;
    currPos  = 0;
    currNode = list->first;
  }

  while (currNode != NULL) {
    if (currPos == pos)
      break;

    currNode = (reverse ? (currNode->prev) : (currNode->next));
    currPos  = (reverse ? (currPos-1) : (currPos+1));
  }
  return currNode;
}

// add an element to the end of a list
// takes in a pointer to a list and a pointer to the data
// returns 0 on success, -1 on failure
int gll_push_end(gll_t *list, void *data) {
  // initialize new node
  gll_node_t *newNode = gll_init_node(data);

  // if list is empty
  if (list->size == 0) {
    list->first = newNode;
  }
  else {
    // if there is at least one element
    list->last->next = newNode;
    newNode->prev = list->last;
  }
  list->last = newNode;
  list->size++;
  return 0;
}

// remove a node from an arbitrary position
// takes in a pointer to a list and a poiner to data
// return 0 on success, -1 on failure
void *gll_remove(gll_t *list, int pos) {
  gll_node_t *currNode = gll_find_node(list, pos);
  void *data = NULL;

  if(currNode == NULL)
    return NULL;

  data = currNode->data;

  if(currNode->prev == NULL)
    list->first = currNode->next;
  else
    currNode->prev->next = currNode->next;

  if(currNode->next == NULL)
    list->last = currNode->prev;
  else
    currNode->next->prev = currNode->prev;

  list->size--;
  free(currNode);
  return data;
}

// destroys a list a frees all list realted memory, but not data stored at nodes
// takes in a pointer to a list
void gll_destroy(gll_t *list) {
  gll_node_t *currNode = list->first;
  gll_node_t *nextNode;

  while (currNode != NULL) {
    nextNode = currNode->next;
    free(currNode);
    currNode = nextNode;
  }
  free(list);
}
