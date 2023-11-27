#include <linux/slab.h>
#include <linux/printk.h>

typedef struct Node {
    int data;
    struct Node* next;
} Node;

typedef struct {
    Node* head;
} CircularQueue;

void initQueue(CircularQueue* q);

void setNodeValue(CircularQueue* q, int value);

int getNodeValue(CircularQueue* q);

void freeQueue(CircularQueue* q);

#endif
