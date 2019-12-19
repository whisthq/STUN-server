/*
 * This file contains the headers of a simple generic doubly-linked list
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

// generic linked list node type
struct gll_node_t {
  void *data;
  struct gll_node_t *prev;
  struct gll_node_t *next;
};

// generic linked list type
struct gll_t {
  int size;
  struct gll_node_t *first;
  struct gll_node_t *last;
};

// initialize a new linked list
// returns a pointer to the new list
struct gll_t *gll_init(void);

// initialize a new node
// takes in a pointer to the data
// returns a pointer to the new node
struct gll_node_t *gll_init_node(void *data);

// find node at a given position
// takes in a pointer to a list and a position
// returns a pointer to the node or NULL on failure
struct gll_node_t *gll_find_node(struct gll_t *list, int pos);

// add an element to the end of a list
// takes in a pointer to a list and a pointer to the data
// returns 0 on success, -1 on failure
int gll_push_end(struct gll_t *list, void *data);

// remove a node from an arbitrary position
// takes in a pointer to a list and a poiner to data
// return 0 on success, -1 on failure
void *gll_remove(struct gll_t *list, int pos);

// destroys a list a frees all list realted memory, but not data stored at nodes
// takes in a pointer to a list
void gll_destroy(struct gll_t *list);
