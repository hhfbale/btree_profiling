#include "cbtree_cache.h"

void initLinkedList(void* nodep) {
    Node *curr, *previous = NULL;
    Node *first = NULL;
    LinkedList* ll = kmalloc(sizeof(LinkedList), GFP_KERNEL);
    int i; 
    for (i = 0; i < 4; i++) {
        curr = kmalloc(sizeof(Node), GFP_KERNEL);
	//check malloc error
        if (!curr) {
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
        curr->key = kmalloc(sizeof(unsigned long) * 2, GFP_KERNEL);

        if (i == 0) {
            first = curr;
        } else {
            previous->next = curr;
        }

        previous = curr;
    }
    curr->next = first;
    ll->head = first;
    ((unsigned long*)nodep)[0] = (unsigned long)ll;
}

void setcache(unsigned long* leaf_node,struct cbtree_head *head, unsigned long * call_node, unsigned long * key, int arr_len, int key_len) {
    LinkedList* call_node_linkedlist = (LinkedList*)((unsigned long*)call_node)[arr_len];
    if(call_node_linkedlist->head->node != NULL){
        if(call_node_linkedlist->head->node[arr_len + 1] == 1 && call_node_linkedlist->head->node[arr_len + 2] == 1){ //if this cache is last one witch save that node and node already deleted
            freelinkedlist(call_node_linkedlist->head->node,head,arr_len);
            //mempool_free(call_node_linkedlist->head->node, head->mempool);
		}
        else{
			call_node_linkedlist->head->node[arr_len + 1] -= 1;
		}
    }
    leaf_node[arr_len+1] += 1;
    
	call_node_linkedlist->head->node = leaf_node;
    int i;
	for(i = 0;i <key_len; i++ ){
		call_node_linkedlist->head->key[i] = key[i];
    }
    call_node_linkedlist->head = call_node_linkedlist->head->next;
}
void* getNodeValue(void* nodep) {
    LinkedList* ll = (LinkedList*)((unsigned long*)nodep)[0];
    Node* curr = ll->head;
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
    LinkedList* ll = (LinkedList*)((unsigned long*)nodep)[0];
    Node *curr = ll->head;
    
    int i;
    for(i = 0; i < 4;i++){
        if(curr->node != NULL){
		    if(curr->node[arr_len + 2] == 1){
		    }
            else if(cachelongcmp(key, curr->key,arr_len)){
                return curr->node;
            }
        }
        curr = curr->next;

    }
    //there is no target, move curr position to the next of start point and NULL return
    ll->head = ll->head->next;
    return NULL;
}

void freeLinkedList(void* nodep,struct cbtree_head *head, int arr_len) { //arr_len is the length of orignal node
    LinkedList* ll = (LinkedList*)((unsigned long*)nodep)[0];
    Node *curr = ll->head;
    
    if (!curr) {
        return;
    }

    Node *first = curr;
    do {
        Node *temp = curr;
        curr = curr->next;
		if(curr != NULL){
			if(curr->node[arr_len+1] <= 1 && curr->node[arr_len + 2] == 1){ //if this cache is last one witch save that node and node already deleted
				freeLinkedList(curr->node[arr_len],head,arr_len);
                mempool_free(curr->node, head->mempool);
			}
			else{
				curr->node[1] -= 1;
			}
		}
        kfree(temp->key);
        kfree(temp);
    } while (curr != first);
    kfree(ll);
}
