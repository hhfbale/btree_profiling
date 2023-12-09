#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cache.h>
#include <linux/slab.h>
#include "cbtree_base.h"
#include <linux/pid.h>
#include <linux/random.h>
#include "calclock.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Herman Bale");
MODULE_DESCRIPTION("A module to create a B+ tree with the included bplus datastructure in the Linux source code");

// Define the size of the tree
#define TREE_SIZE 100

struct kmem_cache *cbtree_cachep;

// Declare the B+ tree
struct cbtree_head tree;

struct kmem_cache *cbtree_cachep;

// Initialize map that will keep track of keys in the btree
unsigned long search_counts[TREE_SIZE] = {0};

// Fetch tree geometry
extern struct cbtree_geo cbtree_geo32;

// Define profiling
KTDEF(btree_lookup_iter);

/**
 * @brief Initialized the btree
*/
void create_tree(void){

	cbtree_init(&tree);
}

KTDEF(cbtree_lookup_iter);

/**
 * @brief help function to keep track of how many times each key in tree is searched
 * 
 * @key btree key
*/
void update_search_count(unsigned long key){
    if (key < TREE_SIZE){
        search_counts[key]++;
    }
}

/**
 * @brief Insert a single data_element in btree
 * 
 * @key key corresponding to data_element
*/
void insert_element(unsigned long key){
	unsigned long temp_key[1] = {key}; 
	unsigned long *val;
	val = kmalloc(sizeof(*val), GFP_KERNEL);
	if (!val) {
		printk(KERN_ERR "error\n");
	}
	//unsigned long *val = (unsigned long *)kmalloc(sizeof(unsigned long), GFP_ATOMIC);
	cbtree_insert(&tree, &cbtree_geo32, temp_key, val, GFP_KERNEL);
}

/**
 * @brief fill the btree with data
*/
void fill_tree(void){

	unsigned long i;
	for (i = 1; i <= TREE_SIZE; i++){
		insert_element(i);
	}	
}

/**
 * @brief find data_elemnt and profile btree_lookup
 * 
 * @key potential key for a data_element
 * @return data_element corresponding to key
*/
struct data_element* find_element(unsigned long key){

	printk("to serch %d",key);
	unsigned long temp_key[1] = {key};
	ktime_t localclock[2];
	ktget(&localclock[0]);
	struct data_element *result = cbtree_lookup(&tree, &cbtree_geo32, temp_key);
	ktget(&localclock[1]);
	ktput(localclock, cbtree_lookup_iter);
	return result;
}

/**
 * @brief searches for data elements in a semi-random manner. Skips each key corresponding to the first if-statement and instead search for a random key within the valid range
*/
void find_tree(void){

    unsigned long i;
    unsigned long key;
    
    for (i = 1; i <= TREE_SIZE; i++){
        // Adjust the probability of generating different keys as needed
        if (i % 3 == 0){
            //Generate a random key for 33% of the iterations
			get_random_bytes(&key, sizeof(key));
			key %= TREE_SIZE; // Ensure the key is within the range of your tree
        
		} else{
            // Use a sequential key for the remaining 66% of the iterations
            key = i;
        }

		update_search_count(key);
        find_element(key);	
    }
}

static int __init bplus_module_init(void){

	printk("Initializing bplus_module\n");

	cbtree_cachep = kmem_cache_create("cbtree_node", NODESIZE, 0,
			SLAB_HWCACHE_ALIGN, NULL);
	printk("%d",cbtree_cachep);
	if(!cbtree_cachep)
		printk("fail");
	
	create_tree();
	void * temp = kmem_cache_alloc(cbtree_cachep, GFP_ATOMIC);
	//mempool_alloc(tree.mempool, GFP_ATOMIC);
	//printk("%d",*(tree.mempool));
	int i = 0;
	
	for(i = 0;i < 100; i++){
		insert_element(i);
	}
	/*
	for(i = 0;i < 30; i++){
		printk("loop %d",i);
		find_element(i);
	}
	*/

	for (i = 0; i < 1000; i++) {
        unsigned int random_number;
        get_random_bytes(&random_number, sizeof(random_number));
        random_number = random_number % 100 + 1;  // 1에서 100 사이의 숫자로 변환

        printk("\n\n\n\n\nloop %d, finding element: %d\n", i, random_number);
        find_element(random_number);
    }

	/*
	insert_element(1);
	insert_element(2);
	insert_element(3);
	*/
	// fill_tree();
	return 0;
}

static void __exit bplus_module_exit(void){

	kmem_cache_destroy(cbtree_cachep);

	printk("Exiting bplus_module\n");
	
	/*
	find_element(1);
	find_element(2);
	find_element(3);
	find_element(1);
	find_element(2);
	find_element(3);
	*/
	// unsigned long i;
	// for (i = 0; i < TREE_SIZE; i++){
	// 	if (search_counts[i]){
    //     	printk("Search count for key %lu: %lu\n", i, search_counts[i]);
	// 	}
    // }
	
	// ktprint(2, cbtree_lookup_iter);
	cbtree_destroy(&tree);
}

module_init(bplus_module_init);
module_exit(bplus_module_exit);
