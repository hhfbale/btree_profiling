/* SPDX-License-Identifier: GPL-2.0 */
#ifndef CBTREE_H
#define CBTREE_H

#include <linux/kernel.h>
#include <linux/mempool.h>

/**
 * DOC: B+Tree basics
 *
 * A B+Tree is a data structure for looking up arbitrary (currently allowing
 * unsigned long, u32, u64 and 2 * u64) keys into pointers. The data structure
 * is described at https://en.wikipedia.org/wiki/B-tree, we currently do not
 * use binary search to find the key on lookups.
 *
 * Each B+Tree consists of a head, that contains bookkeeping information and
 * a variable number (starting with zero) nodes. Each node contains the keys
 * and pointers to sub-nodes, or, for leaf nodes, the keys and values for the
 * tree entries.
 *
 * Each node in this implementation has the following layout:
 * [key1, key2, ..., keyN] [val1, val2, ..., valN]
 *
 * Each key here is an array of unsigned longs, geo->no_longs in total. The
 * number of keys and values (N) is geo->no_pairs.
 */

/**
 * struct cbtree_head - cbtree head
 *
 * @node: the first node in the tree
 * @mempool: mempool used for node allocations
 * @height: current of the tree
 */
struct cbtree_head {
	unsigned long *node;
	mempool_t *mempool;
	int height;
};

/* cbtree geometry */
struct cbtree_geo;

/**
 * cbtree_alloc - allocate function for the mempool
 * @gfp_mask: gfp mask for the allocation
 * @pool_data: unused
 */
void *cbtree_alloc(gfp_t gfp_mask, void *pool_data);

/**
 * cbtree_free - free function for the mempool
 * @element: the element to free
 * @pool_data: unused
 */
void cbtree_free(void *element, void *pool_data);

/**
 * cbtree_init_mempool - initialise a cbtree with given mempool
 *
 * @head: the cbtree head to initialise
 * @mempool: the mempool to use
 *
 * When this function is used, there is no need to destroy
 * the mempool.
 */
void cbtree_init_mempool(struct cbtree_head *head, mempool_t *mempool);

/**
 * cbtree_init - initialise a cbtree
 *
 * @head: the cbtree head to initialise
 *
 * This function allocates the memory pool that the
 * cbtree needs. Returns zero or a negative error code
 * (-%ENOMEM) when memory allocation fails.
 *
 */
int __must_check cbtree_init(struct cbtree_head *head);

/**
 * cbtree_destroy - destroy mempool
 *
 * @head: the cbtree head to destroy
 *
 * This function destroys the internal memory pool, use only
 * when using cbtree_init(), not with cbtree_init_mempool().
 */
void cbtree_destroy(struct cbtree_head *head);

/**
 * cbtree_lookup - look up a key in the cbtree
 *
 * @head: the cbtree to look in
 * @geo: the cbtree geometry
 * @key: the key to look up
 *
 * This function returns the value for the given key, or %NULL.
 */
void *cbtree_lookup(struct cbtree_head *head, struct cbtree_geo *geo,
		   unsigned long *key);

/**
 * cbtree_insert - insert an entry into the cbtree
 *
 * @head: the cbtree to add to
 * @geo: the cbtree geometry
 * @key: the key to add (must not already be present)
 * @val: the value to add (must not be %NULL)
 * @gfp: allocation flags for node allocations
 *
 * This function returns 0 if the item could be added, or an
 * error code if it failed (may fail due to memory pressure).
 */
int __must_check cbtree_insert(struct cbtree_head *head, struct cbtree_geo *geo,
			      unsigned long *key, void *val, gfp_t gfp);
/**
 * cbtree_update - update an entry in the cbtree
 *
 * @head: the cbtree to update
 * @geo: the cbtree geometry
 * @key: the key to update
 * @val: the value to change it to (must not be %NULL)
 *
 * This function returns 0 if the update was successful, or
 * -%ENOENT if the key could not be found.
 */
int cbtree_update(struct cbtree_head *head, struct cbtree_geo *geo,
		 unsigned long *key, void *val);
/**
 * cbtree_remove - remove an entry from the cbtree
 *
 * @head: the cbtree to update
 * @geo: the cbtree geometry
 * @key: the key to remove
 *
 * This function returns the removed entry, or %NULL if the key
 * could not be found.
 */
void *cbtree_remove(struct cbtree_head *head, struct cbtree_geo *geo,
		   unsigned long *key);

/**
 * cbtree_merge - merge two cbtrees
 *
 * @target: the tree that gets all the entries
 * @victim: the tree that gets merged into @target
 * @geo: the cbtree geometry
 * @gfp: allocation flags
 *
 * The two trees @target and @victim may not contain the same keys,
 * that is a bug and triggers a BUG(). This function returns zero
 * if the trees were merged successfully, and may return a failure
 * when memory allocation fails, in which case both trees might have
 * been partially merged, i.e. some entries have been moved from
 * @victim to @target.
 */
int cbtree_merge(struct cbtree_head *target, struct cbtree_head *victim,
		struct cbtree_geo *geo, gfp_t gfp);

/**
 * cbtree_last - get last entry in cbtree
 *
 * @head: cbtree head
 * @geo: cbtree geometry
 * @key: last key
 *
 * Returns the last entry in the cbtree, and sets @key to the key
 * of that entry; returns NULL if the tree is empty, in that case
 * key is not changed.
 */
void *cbtree_last(struct cbtree_head *head, struct cbtree_geo *geo,
		 unsigned long *key);

/**
 * cbtree_get_prev - get previous entry
 *
 * @head: cbtree head
 * @geo: cbtree geometry
 * @key: pointer to key
 *
 * The function returns the next item right before the value pointed to by
 * @key, and updates @key with its key, or returns %NULL when there is no
 * entry with a key smaller than the given key.
 */
void *cbtree_get_prev(struct cbtree_head *head, struct cbtree_geo *geo,
		     unsigned long *key);


/* internal use, use cbtree_visitor{l,32,64,128} */
size_t cbtree_visitor(struct cbtree_head *head, struct cbtree_geo *geo,
		     unsigned long opaque,
		     void (*func)(void *elem, unsigned long opaque,
				  unsigned long *key, size_t index,
				  void *func2),
		     void *func2);

/* internal use, use cbtree_grim_visitor{l,32,64,128} */
size_t cbtree_grim_visitor(struct cbtree_head *head, struct cbtree_geo *geo,
			  unsigned long opaque,
			  void (*func)(void *elem, unsigned long opaque,
				       unsigned long *key,
				       size_t index, void *func2),
			  void *func2);

extern struct kmem_cache *cbtree_cachep;

#include "cbtree-128.h"

extern struct cbtree_geo cbtree_geo32;
#define BTREE_TYPE_SUFFIX l
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define NODESIZE MAX(L1_CACHE_BYTES, 128)
#define CBTREE_TYPE_SUFFIX l
#define CBTREE_TYPE_BITS BITS_PER_LONG
#define CBTREE_TYPE_GEO &cbtree_geo32
#define CBTREE_KEYTYPE unsigned long
#include "cbtree-type.h"

#define cbtree_for_each_safel(head, key, val)	\
	for (val = cbtree_lastl(head, &key);	\
	     val;				\
	     val = cbtree_get_prevl(head, &key))

#define CBTREE_TYPE_SUFFIX 32
#define CBTREE_TYPE_BITS 32
#define CBTREE_TYPE_GEO &cbtree_geo32
#define CBTREE_KEYTYPE u32
#include "cbtree-type.h"

#define cbtree_for_each_safe32(head, key, val)	\
	for (val = cbtree_last32(head, &key);	\
	     val;				\
	     val = cbtree_get_prev32(head, &key))

extern struct cbtree_geo cbtree_geo64;
#define CBTREE_TYPE_SUFFIX 64
#define CBTREE_TYPE_BITS 64
#define CBTREE_TYPE_GEO &cbtree_geo64
#define CBTREE_KEYTYPE u64
#include "cbtree-type.h"

#define cbtree_for_each_safe64(head, key, val)	\
	for (val = cbtree_last64(head, &key);	\
	     val;				\
	     val = cbtree_get_prev64(head, &key))

#endif
