/*
 * skiplistdb
 *
 *
 * skiplistdb is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 *
 * Copyright (c) 2017 Partha Susarla <mail@spartha.org>
 *
 */

#ifndef _BTREE_H_
#define _BTREE_H_

#include <stdio.h>
#include <stdint.h>

#define BTREE_MAX_ELEMENTS 10
#define BTREE_MIN_ELEMENTS 5

enum NodeType {
        LEAF_NODE,
        INTERNAL_NODE,
};

/* Return codes */
enum {
        BTREE_OK             =  0,
        BTREE_ERROR          = -1,
        BTREE_INVALID        = -2,
        BTREE_DUPLICATE      = -3,
        BTREE_NOT_FOUND      = -4,
};


struct btree_node {
        struct btree_node *parent;

        uint32_t count;
        uint32_t depth;

        uint32_t pos;

        const void *keys[BTREE_MAX_ELEMENTS];
        size_t keylens[BTREE_MAX_ELEMENTS];
        const void *vals[BTREE_MAX_ELEMENTS];

        struct btree_node *branches[];
};

struct btree_iter {
        struct btree *tree;
        struct btree_node *node;

        uint32_t pos;

        void *key;
        size_t keylen;
        void *val;
};

typedef struct btree_iter btree_iter_t[1];


/** Callbacks **/
typedef int (*btree_action_cb_t)(void *record, void *data);
typedef unsigned int (*btree_search_cb_t)(void *key, size_t keylen,
                                          const void * const *base,
                                          unsigned int count,
                                          int *found);

struct btree {
        struct btree_node *root;
        size_t count;

        btree_action_cb_t destroy;
        void *destroy_data;

        btree_search_cb_t search;
};

/* btree_new():
 * Creates a new btree. Takes two arguments for callbacks.
 * They can be NULL, in which case, it defaults to using the default delete
 * and search functions, which operate on `unsigned char`.
 */
struct btree *btree_new(btree_action_cb_t destroy, btree_search_cb_t search);

void btree_free(struct btree *tree);

/* btree_insert():
 * Returns:
 *   On Success - returns BTREE_OK
 *   On Failure - returns non 0
 */
int btree_insert(struct btree *tree, void *key, size_t keylen,
                 const void *record);

/* btree_insert_at():
 * Insert a record before the one pointed to by iter
 */
void btree_insert_at(btree_iter_t iter, void *key, size_t keylen,
                     const void *record);

/* btree_remove():
 * Returns:
 *   On Success - returns BTREE_OK
 *   On Failure - returns non 0
 */
int btree_remove(struct btree *tree, void *key, size_t keylen);

/* btree_remove_at():
 * Removes the record pointed to by the iter. This function invalidates the
 * iter.
 */
int btree_remove_at(btree_iter_t iter);

/* btree_deref():
 */
int btree_deref(btree_iter_t iter);

/* Lookup/Find functions */
int btree_lookup(struct btree *tree, const void *key);
/* btree_find():
 * Return value:
 *  On Success: returns 1 with iter->element containing the match
 *  On Failure: returns 0
 */
int btree_find(struct btree *tree, void *key, size_t keylen,
               btree_iter_t iter);

/* These are the default callbacks that are used in the absence of
 * callbacks from the user.
 */

/* The B-Tree requires a binary search function for comparison.
 */
unsigned int btree_memcmp(void *key, size_t keylen,
                          const void * const *base,
                          unsigned int count, int *found);

int btree_destroy(void *record, void *data);

#endif  /* _BTREE_H_ */