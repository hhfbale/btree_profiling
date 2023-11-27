#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/btree.h>
#include <linux/pid.h>
#include "calclock.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Herman Bale");
MODULE_DESCRIPTION("A module to create a B+ tree with the included bplus datastructure in the Linux source code");

// Declare the B+ tree
struct btree_head tree;

// Fetch tree geometry
extern struct btree_geo btree_geo32;

// Define leaf node structure
struct data_element {
	unsigned long key;
};

void create_tree(void){

	btree_init(&tree);
}

KTDEF(btree_lookup_iter);

void insert_element(unsigned long key){

	struct data_element insert_data = {.key = key};

	btree_insert(&tree, &btree_geo32, &key, &insert_data, GFP_KERNEL);

	printk("Inserted key %ld in B+ tree\n", key);
}

struct data_element* find_element(unsigned long key){

	ktime_t localclock[2];
	ktget(&localclock[0]);
	struct data_element *result = btree_lookup(&tree, &btree_geo32, &key);
	ktget(&localclock[1]);
	ktput(localclock, btree_lookup_iter);


	if (result){
		printk("Found key %ld in B+ tree\n", key);
	}
	else{
		printk("Key %ld not in B+ tree\n", key);
	}
	
	return result;
}

static int __init bplus_module_init(void){
	printk("Initializing bplus_module\n");
	
	create_tree();
	insert_element(1);
	return 0;
}

static void __exit bplus_module_exit(void){
	printk("Exiting bplus_module\n");

	find_element(1);

	ktprint(2, btree_lookup_iter);
	btree_destroy(&tree);
}


module_init(bplus_module_init);
module_exit(bplus_module_exit);