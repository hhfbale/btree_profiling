#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/btree.h>
#include <linux/pid.h>
#include <linux/random.h>
#include "calclock.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Herman Bale");
MODULE_DESCRIPTION("A module to create a B+ tree with the included bplus datastructure in the Linux source code");

#define tree_size 100
// Declare the B+ tree
struct btree_head tree;
//unsigned long tree_size = 50;
unsigned long search_counts[tree_size] = {0};

// Fetch tree geometry
extern struct btree_geo btree_geo32;

// Define leaf node structure
struct data_element {
	unsigned long key;
};

void create_tree(void){

	btree_init(&tree);
}

void update_search_count(unsigned long key) {
    if (key < tree_size) {
        search_counts[key]++;
    }
}

KTDEF(btree_lookup_iter);

void insert_element(unsigned long key){

	struct data_element insert_data = {.key = key};

	btree_insert(&tree, &btree_geo32, &key, &insert_data, GFP_KERNEL);

	//printk("Inserted key %ld in B+ tree\n", key);
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
	struct data_element *result = btree_lookup(&tree, &btree_geo32, &key);
	ktget(&localclock[1]);
	ktput(localclock, btree_lookup_iter);


	// if (result){
	// 	printk("Found key %ld in B+ tree\n", key);
	// }
	// else{
	// 	printk("Key %ld not in B+ tree\n", key);
	// }
	
	return result;
}

void find_tree(void) {
    unsigned long i;
    unsigned long key;
    
    // Use the kernel's random number generator to introduce some randomness
    for (i = 1; i <= tree_size; i++) {
        // Adjust the probability of generating different keys as needed
        if (i % 3 == 0) {
            //Generate a random key for 33% of the iterations
			get_random_bytes(&key, sizeof(key));
			key %= tree_size; // Ensure the key is within the range of your tree
        
		} else {
            // Use a sequential key for the remaining 66% of the iterations
            key = i;
        }

		update_search_count(key);
        find_element(key);
		
    }
}

static int __init bplus_module_init(void){
	printk("Initializing bplus_module\n");
	
	create_tree();
	fill_tree();
	
	return 0;
}

static void __exit bplus_module_exit(void){
	printk("Exiting bplus_module\n");
	unsigned long i;

	find_tree();
	for (i = 0; i < tree_size; i++) {
		if (search_counts[i]){
        	printk("Search count for key %lu: %lu\n", i, search_counts[i]);
		}
    }
	ktprint(2, btree_lookup_iter);
	btree_destroy(&tree);
}


module_init(bplus_module_init);
module_exit(bplus_module_exit);