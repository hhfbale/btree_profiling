/* SPDX-License-Identifier: GPL-2.0 */
#define __CBTREE_TP(pfx, type, sfx)	pfx ## type ## sfx
#define _CBTREE_TP(pfx, type, sfx)	__CBTREE_TP(pfx, type, sfx)
#define CBTREE_TP(pfx)			_CBTREE_TP(pfx, CBTREE_TYPE_SUFFIX,)
#define CBTREE_FN(name)			CBTREE_TP(cbtree_ ## name)
#define CBTREE_TYPE_HEAD			CBTREE_TP(struct cbtree_head)
#define VISITOR_FN			CBTREE_TP(visitor)
#define VISITOR_FN_T			_CBTREE_TP(visitor, CBTREE_TYPE_SUFFIX, _t)

CBTREE_TYPE_HEAD {
	struct cbtree_head h;
};

static inline void CBTREE_FN(init_mempool)(CBTREE_TYPE_HEAD *head,
					  mempool_t *mempool)
{
	cbtree_init_mempool(&head->h, mempool);
}

static inline int CBTREE_FN(init)(CBTREE_TYPE_HEAD *head)
{
	return cbtree_init(&head->h);
}

static inline void CBTREE_FN(destroy)(CBTREE_TYPE_HEAD *head)
{
	cbtree_destroy(&head->h);
}

static inline int CBTREE_FN(merge)(CBTREE_TYPE_HEAD *target,
				  CBTREE_TYPE_HEAD *victim,
				  gfp_t gfp)
{
	return cbtree_merge(&target->h, &victim->h, CBTREE_TYPE_GEO, gfp);
}

#if (BITS_PER_LONG > CBTREE_TYPE_BITS)
static inline void *CBTREE_FN(lookup)(CBTREE_TYPE_HEAD *head, CBTREE_KEYTYPE key)
{
	unsigned long _key = key;
	return cbtree_lookup(&head->h, CBTREE_TYPE_GEO, &_key);
}

static inline int CBTREE_FN(insert)(CBTREE_TYPE_HEAD *head, CBTREE_KEYTYPE key,
				   void *val, gfp_t gfp)
{
	unsigned long _key = key;
	return cbtree_insert(&head->h, CBTREE_TYPE_GEO, &_key, val, gfp);
}

static inline int CBTREE_FN(update)(CBTREE_TYPE_HEAD *head, CBTREE_KEYTYPE key,
		void *val)
{
	unsigned long _key = key;
	return cbtree_update(&head->h, CBTREE_TYPE_GEO, &_key, val);
}

static inline void *CBTREE_FN(remove)(CBTREE_TYPE_HEAD *head, CBTREE_KEYTYPE key)
{
	unsigned long _key = key;
	return cbtree_remove(&head->h, CBTREE_TYPE_GEO, &_key);
}

static inline void *CBTREE_FN(last)(CBTREE_TYPE_HEAD *head, CBTREE_KEYTYPE *key)
{
	unsigned long _key;
	void *val = cbtree_last(&head->h, CBTREE_TYPE_GEO, &_key);
	if (val)
		*key = _key;
	return val;
}

static inline void *CBTREE_FN(get_prev)(CBTREE_TYPE_HEAD *head, CBTREE_KEYTYPE *key)
{
	unsigned long _key = *key;
	void *val = cbtree_get_prev(&head->h, CBTREE_TYPE_GEO, &_key);
	if (val)
		*key = _key;
	return val;
}
#else
static inline void *CBTREE_FN(lookup)(CBTREE_TYPE_HEAD *head, CBTREE_KEYTYPE key)
{
	return cbtree_lookup(&head->h, CBTREE_TYPE_GEO, (unsigned long *)&key);
}

static inline int CBTREE_FN(insert)(CBTREE_TYPE_HEAD *head, CBTREE_KEYTYPE key,
			   void *val, gfp_t gfp)
{
	return cbtree_insert(&head->h, CBTREE_TYPE_GEO, (unsigned long *)&key,
			    val, gfp);
}

static inline int CBTREE_FN(update)(CBTREE_TYPE_HEAD *head, CBTREE_KEYTYPE key,
		void *val)
{
	return cbtree_update(&head->h, CBTREE_TYPE_GEO, (unsigned long *)&key, val);
}

static inline void *CBTREE_FN(remove)(CBTREE_TYPE_HEAD *head, CBTREE_KEYTYPE key)
{
	return cbtree_remove(&head->h, CBTREE_TYPE_GEO, (unsigned long *)&key);
}

static inline void *CBTREE_FN(last)(CBTREE_TYPE_HEAD *head, CBTREE_KEYTYPE *key)
{
	return cbtree_last(&head->h, CBTREE_TYPE_GEO, (unsigned long *)key);
}

static inline void *CBTREE_FN(get_prev)(CBTREE_TYPE_HEAD *head, CBTREE_KEYTYPE *key)
{
	return cbtree_get_prev(&head->h, CBTREE_TYPE_GEO, (unsigned long *)key);
}
#endif

void VISITOR_FN(void *elem, unsigned long opaque, unsigned long *key,
		size_t index, void *__func);

typedef void (*VISITOR_FN_T)(void *elem, unsigned long opaque,
			     CBTREE_KEYTYPE key, size_t index);

static inline size_t CBTREE_FN(visitor)(CBTREE_TYPE_HEAD *head,
				       unsigned long opaque,
				       VISITOR_FN_T func2)
{
	return cbtree_visitor(&head->h, CBTREE_TYPE_GEO, opaque,
			     visitorl, func2);
}

static inline size_t CBTREE_FN(grim_visitor)(CBTREE_TYPE_HEAD *head,
					    unsigned long opaque,
					    VISITOR_FN_T func2)
{
	return cbtree_grim_visitor(&head->h, CBTREE_TYPE_GEO, opaque,
				  visitorl, func2);
}

#undef VISITOR_FN
#undef VISITOR_FN_T
#undef __CBTREE_TP
#undef _CBTREE_TP
#undef CBTREE_TP
#undef CBTREE_FN
#undef CBTREE_TYPE_HEAD
#undef CBTREE_TYPE_SUFFIX
#undef CBTREE_TYPE_GEO
#undef CBTREE_KEYTYPE
#undef CBTREE_TYPE_BITS
