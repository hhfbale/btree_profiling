#include <linux/slab.h>
#include <linux/printk.h>
#include "cbtree_base.h"

typedef struct Node {
    unsigned long *node;
    unsigned long *key;
    struct Node* next;
} Node;

typedef struct {
    Node* head;
} CircularQueue;

void initQueue(CircularQueue* q);

void setcache(CircularQueue* q,struct cbtree_head *head, unsigned long * node, unsigned long * key, int arr_len, int key_len);

void* getNodeValue(CircularQueue* q);

static int cachelongcmp(const unsigned long *l1, const unsigned long *l2, size_t n);

void* findNode(CircularQueue* q, unsigned long* key, struct cbtree_head *head, int arr_len);

void freeQueue(CircularQueue* q,struct cbtree_head *head, int arr_len);
