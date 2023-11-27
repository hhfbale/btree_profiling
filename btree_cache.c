#include <linux/slab.h>
#include <linux/printk.h>
#include "btree_cache.h"

typedef struct Node {
    int data;
    struct Node* next;
} Node;

typedef struct {
    Node* nodes[4];
} CircularQueue;

void initQueue(CircularQueue* q) {
    for (int i = 0; i < 4; i++) {
        q->nodes[i] = kmalloc(sizeof(Node), GFP_KERNEL);
        if (!q->nodes[i]) {
            printk(KERN_ERR "Memory allocation failed for node %d\n", i);
            return;
        }
        q->nodes[i]->data = 0;  
    }

    for (int i = 0; i < 3; i++) {
        q->nodes[i]->next = q->nodes[i + 1];
    }
    q->nodes[3]->next = q->nodes[0]; 
}


void setNodeValue(CircularQueue* q, int index, int value) {
    if (index < 0 || index > 3) {
        printk(KERN_INFO "Index out of range\n");
        return;
    }
    q->nodes[index]->data = value;
}


int getNodeValue(CircularQueue* q, int index) {
    if (index < 0 || index > 3) {
        printk(KERN_INFO "Index out of range\n");
        return -1;
    }
    return q->nodes[index]->data;
}


void freeQueue(CircularQueue* q) {
    for (int i = 0; i < 4; i++) {
        if (q->nodes[i]) {
            kfree(q->nodes[i]);
        }
    }
}
