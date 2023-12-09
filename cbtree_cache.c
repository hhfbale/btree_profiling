#include "cbtree_cache.h"

void initQueue(void* nodep) {
    Node *curr, *previous = NULL;
    Node *first = NULL;
    printk("%d", nodep);
    //CircularQueue* q = (CircularQueue*)nodep;
    CircularQueue* q = kmalloc(sizeof(CircularQueue), GFP_KERNEL);
    int i; 
    for (i = 0; i < 4; i++) {
        curr = kmalloc(sizeof(Node), GFP_KERNEL);
        printk("\nnew cache node created %p\n",curr);
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
        curr->key = kmalloc(sizeof(unsigned long)*2, GFP_KERNEL);
		

        if (i == 0) {
            first = curr;
        } else {
            previous->next = curr;
        }

        previous = curr;
    }
    curr->next = first;
    q->head = first;
    ((unsigned long*)nodep)[0] = (unsigned long)q;
    printk("strat cache node of queue : %p",q->head);
    printk("strat cache node of queue : %p",((unsigned long*)nodep)[0]);
    printk("*nodep %d", ((unsigned long*)nodep)[0] );
    printk("q %d", q);
    printk("q-head %d", q->head);
}

void setcache(void* nodep,struct cbtree_head *head, unsigned long * node, unsigned long * key, int arr_len, int key_len) {
	//CircularQueue* q = (CircularQueue*)*nodep;
    CircularQueue* q = (CircularQueue*)((unsigned long*)nodep)[0];
    printk("setcache get %d", nodep);
    Node* curr = q->head;
    //printk("set cache call %d",curr->node);
	if(curr->node != NULL){
		if(curr->node[1] == 1 && curr->node[2] == 1){ //if this cache is last one witch save that node and node already deleted
			mempool_free(curr->node, head->mempool);
		}
		else{
			curr->node[1] -= 1;
		}
	}
	curr->node = node;
	curr->node[1] += 1;

    int i;
	for(i = 0;i <key_len; i++ ){
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

void* getNodeValue(void* nodep) {
    //CircularQueue* q = (CircularQueue*)*nodep;
    CircularQueue* q = (CircularQueue*)((unsigned long*)nodep)[0];
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

void* findNode(void* nodep, unsigned long* key, struct cbtree_head *head, int arr_len) {
    //CircularQueue* q = (CircularQueue*)*nodep;
    CircularQueue* q = (CircularQueue*)((unsigned long*)nodep)[0];
    printk("findNode %d", nodep);
    Node *curr = q->head;
    printk("find cache call curr %d",curr);
    printk("find cache call curr->node %d",curr->node);
    Node *first = curr;
    
    int i;
    if(curr->node != NULL){
        for(i = 0; i < 4;i++){
        printk("%d",i);
		if(curr->node[2] == 1){
            printk("if called");
			mempool_free(curr->node, head->mempool);
		}
        else if(cachelongcmp(key, curr->key,arr_len)){
            printk("else if called");
            return curr->node;
        }
        q->head = curr->next;
        }
    }
    /*
    for(i = 0; i < 4;i++){
        printk("%d",i);
		if(curr->node[2] == 1){
            printk("if called");
			mempool_free(curr->node, head->mempool);
		}
        else if(cachelongcmp(key, curr->key,arr_len)){
            printk("else if called");
            return curr->node;
        }
        q->head = curr->next;
    }
    */
    //there is no target, move curr position to the next of start point and NULL return
    curr = curr->next;
    return NULL;
}

void freeQueue(void* nodep,struct cbtree_head *head, int arr_len) { //arr_len is the length of orignal node
    //CircularQueue* q = (CircularQueue*)*nodep;
    CircularQueue* q = (CircularQueue*)((unsigned long*)nodep)[0];
    printk("freequeue %d", nodep);
    Node *curr = q->head;
    
    if (!curr) {
        return;
    }

    Node *first = curr;
    do {
        Node *temp = curr;
        curr = curr->next;
		if(curr != NULL){
			if(curr->node[1] == 1 && curr->node[2] == 1){ //if this cache is last one witch save that node and node already deleted
				mempool_free(curr->node, head->mempool);
			}
			else{
				curr->node[1] -= 1;
			}
		}
        kfree(temp->key);
        kfree(temp);
    } while (curr != first);
}
