#include "cbtree_cache.h"

typedef struct Node {
    unsigned long *node;
    unsigned long *key;
    struct Node* next;
} Node;

typedef struct {
    Node* head;
} CircularQueue;

void initQueue(CircularQueue* q) {
    Node *current, *previous = NULL;
    Node *first = NULL;

    for (int i = 0; i < 4; i++) {
        current = kmalloc(sizeof(Node), GFP_KERNEL);
	
	//check malloc error
        if (!current) {
            printk(KERN_ERR "Memory allocation failed for node %d\n", i);
            current = first;
            while (current) {
                Node *temp = current->next;
                kfree(current);
                current = temp;
            }
            return;
        }

        current->node = NULL;  
        current->next = NULL;
        current->key = current = kmalloc(sizeof(unsigned long)*2, GFP_KERNEL);
		

        if (i == 0) {
            first = current;
        } else {
            previous->next = current;
        }

        previous = current;
    }

    current->next = first;
    q->head = first;
}

void setcache(CircularQueue* q,struct cbtree_head *head, unsigned long * node, unsigned long * key, int arr_len, int key_len) {
	Node* current = q->head;
	if(current != NULL){
		if(current->node[arr_len + 1] == 1 && current->node[arr_len + 2] == 1){ //if this cache is last one witch save that node and node already deleted
			mempool_free(current->node, head->mempool);
		}
		else{
			current->node[arr_len + 1] -= 1;
		}
	}
	current->node = node;
	current->node[arr_len + 1] += 1;
	for(int i = 0;i++ ;i <key_len){
        current->key[i] = key[i];
    }
}

//change node key
/*
void setNodekey(CircularQueue* q, unsigned long * key, unsigned long * c_key, int arr_len) {
    for(int i = 0;i++ ;i <arr_len){
        key[i] = c_key[i];
    }
}
*/

void* getNodeValue(CircularQueue* q) {
    Node* current = q->head;
    return current->node;
}


static int cachelongcmp(const unsigned long *l1, const unsigned long *l2, size_t n)
{
	size_t i;

	for (i = 0; i < n; i++) {
		if (l1[i] != l2[i])
			return 1;
	}
	return 0;
}

void* findNode(CircularQueue* q, unsinged long* key, struct cbtree_head *head) {
    Node *current = q->head;

    Node *first = current;
    
    for(int i = 0; i < 4;i++;){
		if(current->node[arr_len + 2] == 1){
			mempool_free(current->node, head->mempool);
		}
        else if(cachelongcmp(key, current->key)){
            return current->node;
        }
        q->head = current->next;
    }
    //there is no target, move current position to the next of start point and NULL return
    current = current->next;
    return NULL;
}

void freeQueue(CircularQueue* q,struct cbtree_head *head, int arr_len) { //arr_len is the length of orignal node
    Node *current = q->head;

    if (!current) {
        return;
    }

    Node *first = current;
    do {
        Node *temp = current;
        current = current->next;
		if(current != NULL){
			if(current->node[arr_len + 1] == 1 && current->node[arr_len + 2] == 1){ //if this cache is last one witch save that node and node already deleted
				mempool_free(current->node, head->mempool);
			}
			else{
				current->node[arr_len + 1] -= 1;
			}
		}
        kfree(temp->key);
        kfree(temp);
    } while (current != first);
}
