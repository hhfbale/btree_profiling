// SPDX-License-Identifier: GPL-2.0-only
/*
 * lib/cbtree.c	- Simple In-memory B+Tree
 *
 * Copyright (c) 2007-2008 Joern Engel <joern@purestorage.com>
 * Bits and pieces stolen from Peter Zijlstra's code, which is
 * Copyright 2007, Red Hat Inc. Peter Zijlstra
 *
 * see http://programming.kicks-ass.net/kernel-patches/vma_lookup/cbtree.patch
 *
 * A relatively simple B+Tree implementation.  I have written it as a learning
 * exercise to understand how B+Trees work.  Turned out to be useful as well.
 *
 * B+Trees can be used similar to Linux radix trees (which don't have anything
 * in common with textbook radix trees, beware).  Prerequisite for them working
 * well is that access to a random tree node is much faster than a large number
 * of operations within each node.
 *
 * Disks have fulfilled the prerequisite for a long time.  More recently DRAM
 * has gained similar properties, as memory access times, when measured in cpu
 * cycles, have increased.  Cacheline sizes have increased as well, which also
 * helps B+Trees.
 *
 * Compared to radix trees, B+Trees are more efficient when dealing with a
 * sparsely populated address space.  Between 25% and 50% of the memory is
 * occupied with valid pointers.  When densely populated, radix trees contain
 * ~98% pointers - hard to beat.  Very sparse radix trees contain only ~2%
 * pointers.
 *
 * This particular implementation stores pointers identified by a long value.
 * Storing NULL pointers is illegal, lookup will return NULL when no entry
 * was found.
 *
 * A tricks was used that is not commonly found in textbooks.  The lowest
 * values are to the right, not to the left.  All used slots within a node
 * are on the left, all unused slots contain NUL values.  Most operations
 * simply loop once over all slots and terminate on the first NUL.
 */

#include "cbtree_base.h"
#include "cbtree_cache.h"
#include <linux/cache.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>

// #define MAX(a, b) ((a) > (b) ? (a) : (b))
// #define NODESIZE MAX(L1_CACHE_BYTES, 128)
#define CACHE_LENTH 3   // 1 for cache queue 1 for count 1 for to check is it deleted
//#define CACHE_START ->no_pairs + geo->no_longs

struct cbtree_geo {
	int keylen;
	int no_pairs;
	int no_longs;
};

struct cbtree_geo cbtree_geo32 = {
	.keylen = 1,
	.no_pairs = (NODESIZE - CACHE_LENTH) / sizeof(long) / 2,
	.no_longs = (NODESIZE - CACHE_LENTH) / sizeof(long) / 2,
};
EXPORT_SYMBOL_GPL(cbtree_geo32);

#define LONG_PER_U64 (64 / BITS_PER_LONG)
struct cbtree_geo cbtree_geo64 = {
	.keylen = LONG_PER_U64,
	.no_pairs = (NODESIZE - CACHE_LENTH) / sizeof(long) / (1 + LONG_PER_U64),
	.no_longs = LONG_PER_U64 * ((NODESIZE - CACHE_LENTH) / sizeof(long) / (1 + LONG_PER_U64)),
};
EXPORT_SYMBOL_GPL(cbtree_geo64);

struct cbtree_geo cbtree_geo128 = {
	.keylen = 2 * LONG_PER_U64,
	.no_pairs = (NODESIZE - CACHE_LENTH) / sizeof(long) / (1 + 2 * LONG_PER_U64),
	.no_longs = 2 * LONG_PER_U64 * ((NODESIZE - CACHE_LENTH) / sizeof(long) / (1 + 2 * LONG_PER_U64)),
};
EXPORT_SYMBOL_GPL(cbtree_geo128);

#define MAX_KEYLEN	(2 * LONG_PER_U64)

// static struct kmem_cache *cbtree_cachep;

void *cbtree_alloc(gfp_t gfp_mask, void *pool_data)
{	
	printk("%p",&cbtree_cachep);
	return kmem_cache_alloc(cbtree_cachep, gfp_mask);	
	printk("break");
}
EXPORT_SYMBOL_GPL(cbtree_alloc);

void cbtree_free(void *element, void *pool_data)
{
	kmem_cache_free(cbtree_cachep, element);
}
EXPORT_SYMBOL_GPL(cbtree_free);

static unsigned long *cbtree_node_alloc(struct cbtree_head *head, struct cbtree_geo* geo,gfp_t gfp)
{
	unsigned long *node;
	printk("2-1-1");
	printk("%p",&cbtree_cachep);
	node = mempool_alloc(head->mempool, gfp);
	printk("2-1-2");
	if (likely(node))
		memset(node, 0, NODESIZE);
	if (!node) {
    	printk(KERN_ERR "mempool_alloc failed to allocate memory for node\n");
    	return node;
	}
	printk("2-1-3");
	initQueue(&node[geo->keylen * geo->no_pairs + geo->no_longs]);
	printk("2-1-4");
	return node;
}

static int longcmp(const unsigned long *l1, const unsigned long *l2, size_t n)
{
	size_t i;

	for (i = 0; i < n; i++) {
		if (l1[i] < l2[i])
			return -1;
		if (l1[i] > l2[i])
			return 1;
	}
	return 0;
}

static unsigned long *longcpy(unsigned long *dest, const unsigned long *src,
		size_t n)
{
	size_t i;

	for (i = 0; i < n; i++)
		dest[i] = src[i];
	return dest;
}

static unsigned long *longset(unsigned long *s, unsigned long c, size_t n)
{
	size_t i;

	for (i = 0; i < n; i++)
		s[i] = c;
	return s;
}

static void dec_key(struct cbtree_geo *geo, unsigned long *key)
{
	unsigned long val;
	int i;

	for (i = geo->keylen - 1; i >= 0; i--) {
		val = key[i];
		key[i] = val - 1;
		if (val)
			break;
	}
}

static unsigned long *bkey(struct cbtree_geo *geo, unsigned long *node, int n)
{
	return &node[n * geo->keylen];
}

static void *bval(struct cbtree_geo *geo, unsigned long *node, int n)
{
	return (void *)node[geo->no_longs + n];
}

static void setkey(struct cbtree_geo *geo, unsigned long *node, int n,
		   unsigned long *key)
{
	longcpy(bkey(geo, node, n), key, geo->keylen);
}

static void setval(struct cbtree_geo *geo, unsigned long *node, int n,
		   void *val)
{
	node[geo->no_longs + n] = (unsigned long) val;
}

static void clearpair(struct cbtree_geo *geo, unsigned long *node, int n)
{
	longset(bkey(geo, node, n), 0, geo->keylen);
	node[geo->no_longs + n] = 0;
}

static inline void __cbtree_init(struct cbtree_head *head)
{
	head->node = NULL;
	head->height = 0;
}

void cbtree_init_mempool(struct cbtree_head *head, mempool_t *mempool)
{
	__cbtree_init(head);
	head->mempool = mempool;
}
EXPORT_SYMBOL_GPL(cbtree_init_mempool);

int cbtree_init(struct cbtree_head *head)
{
	__cbtree_init(head);
	printk("init done");
	head->mempool = mempool_create(0, cbtree_alloc, cbtree_free, NULL);
	if (!head->mempool)
		printk("NULL returnd");
		return -ENOMEM;
	return 0;
}
EXPORT_SYMBOL_GPL(cbtree_init);

void cbtree_destroy(struct cbtree_head *head)
{
	mempool_free(head->node, head->mempool);
	mempool_destroy(head->mempool);
	head->mempool = NULL;
}
EXPORT_SYMBOL_GPL(cbtree_destroy);

void *cbtree_last(struct cbtree_head *head, struct cbtree_geo *geo,
		 unsigned long *key)
{
	int height = head->height;
	unsigned long *node = head->node;

	if (height == 0)
		return NULL;

	for ( ; height > 1; height--)
		node = bval(geo, node, 0);

	longcpy(key, bkey(geo, node, 0), geo->keylen);
	return bval(geo, node, 0);
}
EXPORT_SYMBOL_GPL(cbtree_last);

static int keycmp(struct cbtree_geo *geo, unsigned long *node, int pos,
		  unsigned long *key)
{
	return longcmp(bkey(geo, node, pos), key, geo->keylen);
}

//binary search for cbtree, it just maked for to search one node's key

static int keyzero(struct cbtree_geo *geo, unsigned long *key)
{
	int i;

	for (i = 0; i < geo->keylen; i++)
		if (key[i])
			return 0;

	return 1;
}

static void *cbtree_lookup_node(struct cbtree_head *head, unsigned long * h_node, struct cbtree_geo *geo,
		unsigned long *key, int height)
{
	int i;
	unsigned long *node = h_node;
	unsigned long *temp_n = NULL;

	if (height == 0)
		return NULL;
	
	///changed code to recersive funtion
	int j;
	for(j = 0 ; j < 4;j++ ){
			temp_n = findNode(&node[geo->keylen * geo->no_pairs + geo->no_longs], key, head, geo->keylen * geo->no_pairs + geo->no_longs);
	}
	if(temp_n != NULL)
		return temp_n;
	for (i = 0; i < geo->no_pairs; i++)
		if (keycmp(geo, node, i, key) <= 0)
			break;
	if (i == geo->no_pairs)
		return NULL;
	node = bval(geo, node, i);
	if (!node)
		return NULL;
	if(height <= 1){
		for (i = 0; i < geo->no_pairs; i++)
			if (keycmp(geo, node, i, key) == 0)
				return node;
		return NULL;
	}
	node = cbtree_lookup_node(head, node, geo, key, height - 1);
	if(node != NULL)
		setcache(&node[geo->keylen * geo->no_pairs + geo->no_longs], head, node, key, geo->keylen * geo->no_pairs + geo->no_longs, geo->keylen);
	
	/*
	for ( ; height > 1; height--) {
		for(int j ; j < 4;j++ ){
			temp_n = getNodeValue((CircularQueue*)node[CACHE_START], key, head);
		}
		if(temp_n != NULL)
			return temp_n;
		for (i = 0; i < geo->no_pairs; i++)
			if (keycmp(geo, node, i, key) <= 0)
				break;
		if (i == geo->no_pairs)
			return NULL;
		setcache((CircularQueue*)node[CACHE_START], head, unsigned long * node, unsigned long * key, unsigned long * c_key, int arr_len, int key_len)
		node = bval(geo, node, i);
		if (!node)
			return NULL;
	}
 	*/
	return node;
}

void *cbtree_lookup(struct cbtree_head *head, struct cbtree_geo *geo,
		unsigned long *key)
{
	int i;
	unsigned long *node;
	
	node = head->node;
	node = cbtree_lookup_node(head, node, geo, key, head->height);
	if (!node)
		return NULL;

	for (i = 0; i < geo->no_pairs; i++)
		if (keycmp(geo, node, i, key) == 0)
			return bval(geo, node, i);
	return NULL;
}
EXPORT_SYMBOL_GPL(cbtree_lookup);

int cbtree_update(struct cbtree_head *head, struct cbtree_geo *geo,
		 unsigned long *key, void *val)
{
	int i;
	unsigned long *node;

	node = cbtree_lookup_node(head,head->node, geo, key,head->height);
	if (!node)
		return -ENOENT;

	for (i = 0; i < geo->no_pairs; i++)
		if (keycmp(geo, node, i, key) == 0) {
			setval(geo, node, i, val);
			return 0;
		}
	return -ENOENT;
}
EXPORT_SYMBOL_GPL(cbtree_update);

/*
 * Usually this function is quite similar to normal lookup.  But the key of
 * a parent node may be smaller than the smallest key of all its siblings.
 * In such a case we cannot just return NULL, as we have only proven that no
 * key smaller than __key, but larger than this parent key exists.
 * So we set __key to the parent key and retry.  We have to use the smallest
 * such parent key, which is the last parent key we encountered.
 */
void *cbtree_get_prev(struct cbtree_head *head, struct cbtree_geo *geo,
		     unsigned long *__key)
{
	int i, height;
	unsigned long *node, *oldnode;
	unsigned long *retry_key = NULL, key[MAX_KEYLEN];

	if (keyzero(geo, __key))
		return NULL;

	if (head->height == 0)
		return NULL;
	longcpy(key, __key, geo->keylen);
retry:
	dec_key(geo, key);

	node = head->node;
	for (height = head->height ; height > 1; height--) {
		for (i = 0; i < geo->no_pairs; i++)
			if (keycmp(geo, node, i, key) <= 0)
				break;
		if (i == geo->no_pairs)
			goto miss;
		oldnode = node;
		node = bval(geo, node, i);
		if (!node)
			goto miss;
		retry_key = bkey(geo, oldnode, i);
	}

	if (!node)
		goto miss;

	for (i = 0; i < geo->no_pairs; i++) {
		if (keycmp(geo, node, i, key) <= 0) {
			if (bval(geo, node, i)) {
				longcpy(__key, bkey(geo, node, i), geo->keylen);
				return bval(geo, node, i);
			} else
				goto miss;
		}
	}
miss:
	if (retry_key) {
		longcpy(key, retry_key, geo->keylen);
		retry_key = NULL;
		goto retry;
	}
	return NULL;
}
EXPORT_SYMBOL_GPL(cbtree_get_prev);
 
static int getpos(struct cbtree_geo *geo, unsigned long *node,
		unsigned long *key)
{
	int i;

	for (i = 0; i < geo->no_pairs; i++) {
		if (keycmp(geo, node, i, key) <= 0)
			break;
	}
	return i;
}

static int getfill(struct cbtree_geo *geo, unsigned long *node, int start)
{
	int i;

	for (i = start; i < geo->no_pairs; i++)
		if (!bval(geo, node, i))
			break;
	return i;
}

/*
 * locate the correct leaf node in the cbtree
 */
static unsigned long *find_level(struct cbtree_head *head, struct cbtree_geo *geo,
		unsigned long *key, int level)
{
	unsigned long *node = head->node;
	int i, height;

	for (height = head->height; height > level; height--) {
		for (i = 0; i < geo->no_pairs; i++)
			if (keycmp(geo, node, i, key) <= 0)
				break;

		if ((i == geo->no_pairs) || !bval(geo, node, i)) {
			/* right-most key is too large, update it */
			/* FIXME: If the right-most key on higher levels is
			 * always zero, this wouldn't be necessary. */
			i--;
			setkey(geo, node, i, key);
		}
		BUG_ON(i < 0);
		node = bval(geo, node, i);
	}
	BUG_ON(!node);
	return node;
}

static int cbtree_grow(struct cbtree_head *head, struct cbtree_geo *geo,
		      gfp_t gfp)
{
	unsigned long *node;
	int fill;
	printk("2-1");
	node = cbtree_node_alloc(head, geo, gfp);
	printk("2-2");
	if (!node)
		return -ENOMEM;
	if (head->node) {
		printk("2-3");
		fill = getfill(geo, head->node, 0);
		setkey(geo, node, 0, bkey(geo, head->node, fill - 1));
		setval(geo, node, 0, head->node);
	}
	printk("2-4");
	head->node = node;
	head->height++;
	return 0;
}

static void cbtree_shrink(struct cbtree_head *head, struct cbtree_geo *geo)
{
	unsigned long *node;
	int fill;

	if (head->height <= 1)
		return;

	node = head->node;
	fill = getfill(geo, node, 0);
	BUG_ON(fill > 1);
	head->node = bval(geo, node, 0);
	head->height--;
	freeQueue(&node[geo->keylen * geo->no_pairs + geo->no_longs], head, geo->keylen * geo->no_pairs + geo->no_longs);
	mempool_free(node, head->mempool);
}

static int cbtree_insert_level(struct cbtree_head *head, struct cbtree_geo *geo,
			      unsigned long *key, void *val, int level,
			      gfp_t gfp)
{
	unsigned long *node;
	int i, pos, fill, err;
	printk("2");
	BUG_ON(!val);
	if (head->height < level) {
		err = cbtree_grow(head, geo, gfp);
		if (err)
			return err;
	}
	printk("3");
retry:
	node = find_level(head, geo, key, level);
	pos = getpos(geo, node, key);
	fill = getfill(geo, node, pos);
	/* two identical keys are not allowed */
	BUG_ON(pos < fill && keycmp(geo, node, pos, key) == 0);
	printk("4");
	if (fill == geo->no_pairs) {
		/* need to split node */
		unsigned long *new;

		new = cbtree_node_alloc(head, geo, gfp);
		if (!new)
			return -ENOMEM;
		err = cbtree_insert_level(head, geo,
				bkey(geo, node, fill / 2 - 1),
				new, level + 1, gfp);
		if (err) {
			mempool_free(new, head->mempool);
			return err;
		}
		for (i = 0; i < fill / 2; i++) {
			setkey(geo, new, i, bkey(geo, node, i));
			setval(geo, new, i, bval(geo, node, i));
			setkey(geo, node, i, bkey(geo, node, i + fill / 2));
			setval(geo, node, i, bval(geo, node, i + fill / 2));
			clearpair(geo, node, i + fill / 2);
		}
		if (fill & 1) {
			setkey(geo, node, i, bkey(geo, node, fill - 1));
			setval(geo, node, i, bval(geo, node, fill - 1));
			clearpair(geo, node, fill - 1);
		}
		goto retry;
	}
	BUG_ON(fill >= geo->no_pairs);
	printk("5");
	/* shift and insert */
	for (i = fill; i > pos; i--) {
		setkey(geo, node, i, bkey(geo, node, i - 1));
		setval(geo, node, i, bval(geo, node, i - 1));
	}
	setkey(geo, node, pos, key);
	setval(geo, node, pos, val);

	return 0;
}

int cbtree_insert(struct cbtree_head *head, struct cbtree_geo *geo,
		unsigned long *key, void *val, gfp_t gfp)
{
	BUG_ON(!val);
	printk("1");
	return cbtree_insert_level(head, geo, key, val, 1, gfp);
}
EXPORT_SYMBOL_GPL(cbtree_insert);

static void *cbtree_remove_level(struct cbtree_head *head, struct cbtree_geo *geo,
		unsigned long *key, int level);
static void merge(struct cbtree_head *head, struct cbtree_geo *geo, int level,
		unsigned long *left, int lfill,
		unsigned long *right, int rfill,
		unsigned long *parent, int lpos)
{
	int i;
	unsigned long *cache_ptr; 									// cache pointer

	
	for (i = 0; i < rfill; i++) {
		/* Move all keys to the left */
		setkey(geo, left, lfill + i, bkey(geo, right, i));
		setval(geo, left, lfill + i, bval(geo, right, i));
	}
	/* Exchange left and right child in parent */
	setval(geo, parent, lpos, right);
	setval(geo, parent, lpos + 1, left);
	/* Remove left (formerly right) child from parent */
	cbtree_remove_level(head, geo, bkey(geo, parent, lpos), level + 1);
	
	////////////////////////// added code to free cache memory
	////////////////////////// in this if statement allocated node really deleted
	cache_ptr = right;
	freeQueue(&cache_ptr[geo->keylen * geo->no_pairs + geo->no_longs],head,geo->keylen * geo->no_pairs + geo->no_longs);
	//////////////////////////cache memory free

	if(cache_ptr[geo->keylen * geo->no_pairs + geo->no_longs + 1] == 0){
		mempool_free(right, head->mempool);
	}
	else{
		cache_ptr[geo->keylen * geo->no_pairs + geo->no_longs + 2] = 1;	
	}
	//not free node, just chang cache state
	//mempool_free(right, head->mempool);    //this is original code
}

static void rebalance(struct cbtree_head *head, struct cbtree_geo *geo,
		unsigned long *key, int level, unsigned long *child, int fill)
{
	unsigned long *parent, *left = NULL, *right = NULL;
	int i, no_left, no_right;
	unsigned long *cache_ptr; 									// cache pointer

	if (fill == 0) {
		/* Because we don't steal entries from a neighbour, this case
		 * can happen.  Parent node contains a single child, this
		 * node, so merging with a sibling never happens.
		 */
		cbtree_remove_level(head, geo, key, level + 1);

		////////////////////////// added code to free cache memory
		////////////////////////// in this if statement allocated node really deleted
		cache_ptr = child;
		freeQueue(&cache_ptr[geo->keylen * geo->no_pairs + geo->no_longs],head,geo->keylen * geo->no_pairs + geo->no_longs);
		//////////////////////////cache memory free

		if(cache_ptr[geo->keylen * geo->no_pairs + geo->no_longs + 1] == 0){
		mempool_free(right, head->mempool);
		}
		else{
			cache_ptr[geo->keylen * geo->no_pairs + geo->no_longs + 2] = 1;	
		}
		//not free node, just chang cache state
		//mempool_free(right, head->mempool);    //this is original code
		return;
	}

	parent = find_level(head, geo, key, level + 1);
	i = getpos(geo, parent, key);
	BUG_ON(bval(geo, parent, i) != child);

	if (i > 0) {
		left = bval(geo, parent, i - 1);
		no_left = getfill(geo, left, 0);
		if (fill + no_left <= geo->no_pairs) {
			merge(head, geo, level,
					left, no_left,
					child, fill,
					parent, i - 1);
			return;
		}
	}
	if (i + 1 < getfill(geo, parent, i)) {
		right = bval(geo, parent, i + 1);
		no_right = getfill(geo, right, 0);
		if (fill + no_right <= geo->no_pairs) {
			merge(head, geo, level,
					child, fill,
					right, no_right,
					parent, i);
			return;
		}
	}
	/*
	 * We could also try to steal one entry from the left or right
	 * neighbor.  By not doing so we changed the invariant from
	 * "all nodes are at least half full" to "no two neighboring
	 * nodes can be merged".  Which means that the average fill of
	 * all nodes is still half or better.
	 */
}

static void *cbtree_remove_level(struct cbtree_head *head, struct cbtree_geo *geo,
		unsigned long *key, int level)
{
	unsigned long *node;
	int i, pos, fill;
	void *ret;

	if (level > head->height) {
		/* we recursed all the way up */
		head->height = 0;
		head->node = NULL;
		return NULL;
	}

	node = find_level(head, geo, key, level);
	pos = getpos(geo, node, key);
	fill = getfill(geo, node, pos);
	if ((level == 1) && (keycmp(geo, node, pos, key) != 0))
		return NULL;
	ret = bval(geo, node, pos);
	
	/* remove and shift */
	for (i = pos; i < fill - 1; i++) {
		setkey(geo, node, i, bkey(geo, node, i + 1));
		setval(geo, node, i, bval(geo, node, i + 1));
	}
		clearpair(geo, node, fill - 1);

	if (fill - 1 < geo->no_pairs / 2) {
		if (level < head->height)
			rebalance(head, geo, key, level, node, fill - 1);
		else if (fill - 1 == 1)
			cbtree_shrink(head, geo);
	}

	return ret;
}

void *cbtree_remove(struct cbtree_head *head, struct cbtree_geo *geo,
		unsigned long *key)
{
	if (head->height == 0)
		return NULL;

	return cbtree_remove_level(head, geo, key, 1);
}
EXPORT_SYMBOL_GPL(cbtree_remove);

int cbtree_merge(struct cbtree_head *target, struct cbtree_head *victim,
		struct cbtree_geo *geo, gfp_t gfp)
{
	unsigned long key[MAX_KEYLEN];
	unsigned long dup[MAX_KEYLEN];
	void *val;
	int err;

	BUG_ON(target == victim);

	if (!(target->node)) {
		/* target is empty, just copy fields over */
		target->node = victim->node;
		target->height = victim->height;
		__cbtree_init(victim);
		return 0;
	}

	/* TODO: This needs some optimizations.  Currently we do three tree
	 * walks to remove a single object from the victim.
	 */
	for (;;) {
		if (!cbtree_last(victim, geo, key))
			break;
		val = cbtree_lookup(victim, geo, key);
		err = cbtree_insert(target, geo, key, val, gfp);
		if (err)
			return err;
		/* We must make a copy of the key, as the original will get
		 * mangled inside cbtree_remove. */
		longcpy(dup, key, geo->keylen);
		cbtree_remove(victim, geo, dup);
	}
	return 0;
}
EXPORT_SYMBOL_GPL(cbtree_merge);

static size_t __cbtree_for_each(struct cbtree_head *head, struct cbtree_geo *geo,
			       unsigned long *node, unsigned long opaque,
			       void (*func)(void *elem, unsigned long opaque,
					    unsigned long *key, size_t index,
					    void *func2),
			       void *func2, int reap, int height, size_t count)
{
	int i;
	unsigned long *child;

	for (i = 0; i < geo->no_pairs; i++) {
		child = bval(geo, node, i);
		if (!child)
			break;
		if (height > 1)
			count = __cbtree_for_each(head, geo, child, opaque,
					func, func2, reap, height - 1, count);
		else
			func(child, opaque, bkey(geo, node, i), count++,
					func2);
	}
	if (reap){
		freeQueue(&node[geo->keylen * geo->no_pairs + geo->no_longs], head, geo->keylen * geo->no_pairs + geo->no_longs);
		mempool_free(node, head->mempool);
	}
	return count;
}

static void empty(void *elem, unsigned long opaque, unsigned long *key,
		  size_t index, void *func2)
{
}

void cvisitorl(void *elem, unsigned long opaque, unsigned long *key,
	      size_t index, void *__func)
{
	visitorl_t func = __func;

	func(elem, opaque, *key, index);
}
EXPORT_SYMBOL_GPL(cvisitorl);

void cvisitor32(void *elem, unsigned long opaque, unsigned long *__key,
	       size_t index, void *__func)
{
	visitor32_t func = __func;
	u32 *key = (void *)__key;

	func(elem, opaque, *key, index);
}
EXPORT_SYMBOL_GPL(cvisitor32);

void cvisitor64(void *elem, unsigned long opaque, unsigned long *__key,
	       size_t index, void *__func)
{
	visitor64_t func = __func;
	u64 *key = (void *)__key;

	func(elem, opaque, *key, index);
}
EXPORT_SYMBOL_GPL(cvisitor64);

void cvisitor128(void *elem, unsigned long opaque, unsigned long *__key,
		size_t index, void *__func)
{
	visitor128_t func = __func;
	u64 *key = (void *)__key;

	func(elem, opaque, key[0], key[1], index);
}
EXPORT_SYMBOL_GPL(cvisitor128);

size_t cbtree_visitor(struct cbtree_head *head, struct cbtree_geo *geo,
		     unsigned long opaque,
		     void (*func)(void *elem, unsigned long opaque,
		     		  unsigned long *key,
		     		  size_t index, void *func2),
		     void *func2)
{
	size_t count = 0;

	if (!func2)
		func = empty;
	if (head->node)
		count = __cbtree_for_each(head, geo, head->node, opaque, func,
				func2, 0, head->height, 0);
	return count;
}
EXPORT_SYMBOL_GPL(cbtree_visitor);

size_t cbtree_grim_visitor(struct cbtree_head *head, struct cbtree_geo *geo,
			  unsigned long opaque,
			  void (*func)(void *elem, unsigned long opaque,
				       unsigned long *key,
				       size_t index, void *func2),
			  void *func2)
{
	size_t count = 0;

	if (!func2)
		func = empty;
	if (head->node)
		count = __cbtree_for_each(head, geo, head->node, opaque, func,
				func2, 1, head->height, 0);
	__cbtree_init(head);
	return count;
}
EXPORT_SYMBOL_GPL(cbtree_grim_visitor);


//static int __init cbtree_module_init(void)
//{
// 	cbtree_cachep = kmem_cache_create("cbtree_node", NODESIZE, 0,
// 			SLAB_HWCACHE_ALIGN, NULL);
//	return 0;
//}

//static void __exit cbtree_module_exit(void)
//{
//	kmem_cache_destroy(cbtree_cachep);
//}

// /* If core code starts using cbtree, initialization should happen even earlier */
// module_init(cbtree_module_init);
// module_exit(cbtree_module_exit);

MODULE_AUTHOR("bae tae hyeon>");
MODULE_LICENSE("GPL");
