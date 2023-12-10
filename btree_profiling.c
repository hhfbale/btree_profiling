#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cache.h>
#include <linux/slab.h>
#include "cbtree_base.h"
#include <linux/pid.h>
#include <linux/random.h>
#include <linux/btree.h>
#include "calclock.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Herman Bale");
MODULE_DESCRIPTION("A module to create a B+ tree with the included bplus datastructure in the Linux source code");

// Define the size of the tree
#define TREE_SIZE 100000

struct kmem_cache *btree_cachep;
struct kmem_cache *cbtree_cachep;

// Declare the B+ tree
struct btree_head btree;
struct cbtree_head cbtree;

// Initialize map that will keep track of keys in the btree
unsigned long search_counts[TREE_SIZE] = {0};

// Fetch tree geometry
extern struct cbtree_geo cbtree_geo32;

/**
 * @brief Initialized the btree
*/
void create_tree(void){
	btree_init(&btree);
	cbtree_init(&cbtree);
}

KTDEF(btree_insert);
KTDEF(btree_lookup);
KTDEF(cbtree_insert);
KTDEF(cbtree_lookup);

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
	unsigned long temp_key_b[1] = {key}; 
	unsigned long temp_key_cb[1] = {key}; 
	unsigned long *val_b;
	unsigned long *val_cb;
	ktime_t stopwatch_b[2];
	ktime_t stopwatch_cb[2];

	val_b = kmalloc(sizeof(*val_b), GFP_KERNEL);
	if (!val_b) {
		printk(KERN_ERR "error\n");
	}
	ktget(&stopwatch_b[0]);
	btree_insert(&btree, &btree_geo32, temp_key_b, val_b, GFP_KERNEL);
	ktget(&stopwatch_b[1]);
	ktput(stopwatch_b, btree_insert);

	val_cb = kmalloc(sizeof(*val_cb), GFP_KERNEL);
	if (!val_cb) {
		printk(KERN_ERR "error\n");
	}
	ktget(&stopwatch_cb[0]);
	cbtree_insert(&cbtree, &cbtree_geo32, temp_key_cb, val_cb, GFP_KERNEL);
	ktget(&stopwatch_cb[1]);
	ktput(stopwatch_cb, cbtree_insert);
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
	unsigned long temp_key_b[1] = {key};
	unsigned long temp_key_cb[1] = {key};
	ktime_t stopwatch_b[2];
	ktime_t stopwatch_cb[2];

	ktget(&stopwatch_b[0]);
	struct data_element *result_b = btree_lookup(&btree, &btree_geo32, temp_key_b);
	ktget(&stopwatch_b[1]);
	ktput(stopwatch_b, btree_lookup);
	
	ktget(&stopwatch_cb[0]);
	struct data_element *result_cb = cbtree_lookup(&cbtree, &cbtree_geo32, temp_key_cb);
	ktget(&stopwatch_cb[1]);
	ktput(stopwatch_cb, cbtree_lookup);

	return result_cb;
}

/**
 * @brief searches for data elements in a semi-random manner. Skips each key corresponding to the first if-statement and instead search for a random key within the valid range
*/
void find_tree(void){

    unsigned long i;
    unsigned long key;
    
    for (i = 1; i <= TREE_SIZE*4; i++){
        // Adjust the probability of generating different keys as needed
        
		//////////////////////////////////////////////////////////
		// get_random_bytes(&key, sizeof(key));	// Random
		key = TREE_SIZE / 2;					// Constant
		// key = i;								// Sequencial
		//////////////////////////////////////////////////////////
		key %= TREE_SIZE; // Ensure the key is within the range of your tree
		if(i%100000 == 0) {
			printk(key);
		}
		update_search_count(key);
		find_element(key);
    }
	printk("ddd");
}

static int __init bplus_module_init(void){

	printk("Initializing bplus_module\n");

	btree_cachep = kmem_cache_create("btree_node", NODESIZE, 0,
			SLAB_HWCACHE_ALIGN, NULL);
	cbtree_cachep = kmem_cache_create("cbtree_node", NODESIZE, 0,
			SLAB_HWCACHE_ALIGN, NULL);

	if(!cbtree_cachep || !btree_cachep)
		printk("fail");
	
	create_tree();
	fill_tree();
	find_tree();
	
	return 0;
}

static void __exit bplus_module_exit(void){
	kmem_cache_destroy(btree_cachep);
	kmem_cache_destroy(cbtree_cachep);

	ktprint(0, cbtree_insert);
	ktprint(0, cbtree_lookup);
	ktprint(0, btree_insert);
	ktprint(0, btree_lookup);

	btree_destroy(&btree);
	cbtree_destroy(&cbtree);
	printk("Exiting bplus_module\n");
}

module_init(bplus_module_init);
module_exit(bplus_module_exit);
