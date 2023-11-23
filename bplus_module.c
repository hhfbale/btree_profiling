#include <linux/init.h>
#include <linux/module.h>
#include <linux/btree.h>
#include <linux/pid.h>

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

void insert_element(unsigned long key){

	struct data_element insert_data = {.key = key};

	btree_insert(&tree, &btree_geo32, &key, &insert_data, GFP_KERNEL);

	printk("Inserted key %ld in B+ tree\n", key);
}

struct data_element* find_element(unsigned long key){

	struct data_element *result = btree_lookup(&tree, &btree_geo32, &key);
	find_ds_monitoring(&thread_dm, current);

	if (result){
		printk("Found key %ld in B+ tree\n", key);
	}
	else{
		printk("Key %ld not in B+ tree\n", key);
	}
	
	return result;
}

static int __init bplus_module_init(void){
	pr_info("Initializing bplus_module\n");
	
	create_tree();
	insert_element(1);
	return 0;
}

static void __exit bplus_module_exit(void){
	printk("Exiting bplus_module\n");

	find_element(1);
	btree_destroy(&tree);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Herman Bale");
MODULE_DESCRIPTION("A module to create a B+ tree with the included bplus datastructure in the Linux source code");

module_init(bplus_module_init);
module_exit(bplus_module_exit);
