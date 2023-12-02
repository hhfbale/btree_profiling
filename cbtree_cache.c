#include "cbtree_cache.h"

void initQueue(CircularQueue* q) {
    Node *curr, *previous = NULL;
    Node *first = NULL;

    int i; 
    for (i = 0; i < 4; i++) {
        curr = kmalloc(sizeof(Node), GFP_KERNEL);
	
	//check malloc error
        if (!curr) {
            printk(KERN_ERR "Memory allocation failed for node %d\n", i);
            curr = first;
            while (curr) {
                Node *temp = curr->next;
                kfree(curr);
                curr = temp;
            }
            return;
        }

        curr->node = NULL;  
        curr->next = NULL;
	curr = kmalloc(sizeof(unsigned long)*2, GFP_KERNEL);
        curr->key = curr;
		

        if (i == 0) {
            first = curr;
        } else {
            previous->next = curr;
        }

        previous = curr;
    }

    curr->next = first;
    q->head = first;
}

void setcache(CircularQueue* q,struct cbtree_head *head, unsigned long * node, unsigned long * key, int arr_len, int key_len) {
	Node* curr = q->head;
	if(curr != NULL){
		if(curr->node[arr_len + 1] == 1 && curr->node[arr_len + 2] == 1){ //if this cache is last one witch save that node and node already deleted
			mempool_free(curr->node, head->mempool);
		}
		else{
			curr->node[arr_len + 1] -= 1;
		}
	}
	curr->node = node;
	curr->node[arr_len + 1] += 1;

    int i;
	for(i = 0;i++ ;i <key_len){
		curr->key[i] = key[i];
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
    Node* curr = q->head;
    return curr->node;
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

void* findNode(CircularQueue* q, unsigned long* key, struct cbtree_head *head) {
    Node *curr = q->head;

    Node *first = curr;
    
    int i;
    for(i = 0; i < 4;i++){
		if(curr->node[arr_len + 2] == 1){
			mempool_free(curr->node, head->mempool);
		}
        else if(cachelongcmp(key, curr->key)){
            return curr->node;
        }
        q->head = curr->next;
    }
    //there is no target, move curr position to the next of start point and NULL return
    curr = curr->next;
    return NULL;
}

void freeQueue(CircularQueue* q,struct cbtree_head *head, int arr_len) { //arr_len is the length of orignal node
    Node *curr = q->head;

    if (!curr) {
        return;
    }

    Node *first = curr;
    do {
        Node *temp = curr;
        curr = curr->next;
		if(curr != NULL){
			if(curr->node[arr_len + 1] == 1 && curr->node[arr_len + 2] == 1){ //if this cache is last one witch save that node and node already deleted
				mempool_free(curr->node, head->mempool);
			}
			else{
				curr->node[arr_len + 1] -= 1;
			}
		}
        kfree(temp->key);
        kfree(temp);
    } while (curr != first);
}
