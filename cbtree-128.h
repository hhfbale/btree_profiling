/* SPDX-License-Identifier: GPL-2.0 */
extern struct cbtree_geo cbtree_geo128;

struct cbtree_head128 { struct cbtree_head h; };

static inline void cbtree_init_mempool128(struct cbtree_head128 *head,
					 mempool_t *mempool)
{
	cbtree_init_mempool(&head->h, mempool);
}

static inline int cbtree_init128(struct cbtree_head128 *head)
{
	return cbtree_init(&head->h);
}

static inline void cbtree_destroy128(struct cbtree_head128 *head)
{
	cbtree_destroy(&head->h);
}

static inline void *cbtree_lookup128(struct cbtree_head128 *head, u64 k1, u64 k2)
{
	u64 key[2] = {k1, k2};
	return cbtree_lookup(&head->h, &cbtree_geo128, (unsigned long *)&key);
}

static inline void *cbtree_get_prev128(struct cbtree_head128 *head,
				      u64 *k1, u64 *k2)
{
	u64 key[2] = {*k1, *k2};
	void *val;

	val = cbtree_get_prev(&head->h, &cbtree_geo128,
			     (unsigned long *)&key);
	*k1 = key[0];
	*k2 = key[1];
	return val;
}

static inline int cbtree_insert128(struct cbtree_head128 *head, u64 k1, u64 k2,
				  void *val, gfp_t gfp)
{
	u64 key[2] = {k1, k2};
	return cbtree_insert(&head->h, &cbtree_geo128,
			    (unsigned long *)&key, val, gfp);
}

static inline int cbtree_update128(struct cbtree_head128 *head, u64 k1, u64 k2,
				  void *val)
{
	u64 key[2] = {k1, k2};
	return cbtree_update(&head->h, &cbtree_geo128,
			    (unsigned long *)&key, val);
}

static inline void *cbtree_remove128(struct cbtree_head128 *head, u64 k1, u64 k2)
{
	u64 key[2] = {k1, k2};
	return cbtree_remove(&head->h, &cbtree_geo128, (unsigned long *)&key);
}

static inline void *cbtree_last128(struct cbtree_head128 *head, u64 *k1, u64 *k2)
{
	u64 key[2];
	void *val;

	val = cbtree_last(&head->h, &cbtree_geo128, (unsigned long *)&key[0]);
	if (val) {
		*k1 = key[0];
		*k2 = key[1];
	}

	return val;
}

static inline int cbtree_merge128(struct cbtree_head128 *target,
				 struct cbtree_head128 *victim,
				 gfp_t gfp)
{
	return cbtree_merge(&target->h, &victim->h, &cbtree_geo128, gfp);
}

void cvisitor128(void *elem, unsigned long opaque, unsigned long *__key,
		size_t index, void *__func);

typedef void (*visitor128_t)(void *elem, unsigned long opaque,
			     u64 key1, u64 key2, size_t index);

static inline size_t cbtree_cvisitor128(struct cbtree_head128 *head,
				      unsigned long opaque,
				      visitor128_t func2)
{
	return cbtree_visitor(&head->h, &cbtree_geo128, opaque,
			     cvisitor128, func2);
}

static inline size_t cbtree_grim_cvisitor128(struct cbtree_head128 *head,
					   unsigned long opaque,
					   visitor128_t func2)
{
	return cbtree_grim_visitor(&head->h, &cbtree_geo128, opaque,
				  cvisitor128, func2);
}

#define cbtree_for_each_safe128(head, k1, k2, val)	\
	for (val = cbtree_last128(head, &k1, &k2);	\
	     val;					\
	     val = cbtree_get_prev128(head, &k1, &k2))

