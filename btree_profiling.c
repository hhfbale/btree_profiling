#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cache.h>
#include <linux/slab.h>
#include "cbtree_base.h"
#include <linux/pid.h>
#include "calclock.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Herman Bale");
MODULE_DESCRIPTION("A module to create a B+ tree with the included bplus datastructure in the Linux source code");

// Declare the B+ tree
struct cbtree_head tree;
unsigned long tree_size = 50;

// Fetch tree geometry
extern struct cbtree_geo cbtree_geo32;

// Define leaf node structure
struct data_element {
	unsigned long key;
};

void create_tree(void){

	cbtree_init(&tree);
}

KTDEF(cbtree_lookup_iter);

void insert_element(unsigned long key){

	struct data_element insert_data = {.key = key};

	cbtree_insert(&tree, &cbtree_geo32, &key, &insert_data, GFP_KERNEL);

	printk("Inserted key %ld in B+ tree\n", key);
}

void fill_tree(void){

	unsigned long i;
	for (i = 1; i <= tree_size; i++)
	{
		insert_element(i);
	}	
}

struct data_element* find_element(unsigned long key){

	ktime_t localclock[2];
	ktget(&localclock[0]);
	struct data_element *result = cbtree_lookup(&tree, &cbtree_geo32, &key);
	ktget(&localclock[1]);
	ktput(localclock, cbtree_lookup_iter);


	if (result){
		printk("Found key %ld in B+ tree\n", key);
	}
	else{
		printk("Key %ld not in B+ tree\n", key);
	}
	
	return result;
}

void find_tree(void){

	unsigned long i;
	for (i = 1; i <= tree_size; i++)
	{
		find_element(i);
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
	
	// find_tree();
	// ktprint(2, cbtree_lookup_iter);
	// cbtree_destroy(&tree);
}


module_init(bplus_module_init);
module_exit(bplus_module_exit);