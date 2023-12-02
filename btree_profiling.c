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

// Define leaf node structure
struct data_element {
	unsigned long key;
};

// Declare the B+ tree
struct cbtree_head tree;

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

	struct data_element insert_data = {.key = key};
	cbtree_insert(&tree, &cbtree_geo32, &key, &insert_data, GFP_KERNEL);
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

	ktime_t localclock[2];
	ktget(&localclock[0]);
	struct data_element *result = cbtree_lookup(&tree, &cbtree_geo32, &key);
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
	
	// create_tree();
	// fill_tree();
	
	return 0;
}

static void __exit bplus_module_exit(void){

	printk("Exiting bplus_module\n");

	kmem_cache_destroy(cbtree_cachep);
	
	find_tree();
	
	unsigned long i;
	for (i = 0; i < TREE_SIZE; i++){
		if (search_counts[i]){
        	printk("Search count for key %lu: %lu\n", i, search_counts[i]);
		}
    }
	
	ktprint(2, cbtree_lookup_iter);
	cbtree_destroy(&tree);
}

module_init(bplus_module_init);
module_exit(bplus_module_exit);