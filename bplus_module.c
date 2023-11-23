#include <linux/init.h>
#include <linux/module.h>
#include <linux/btree.h>
#include <linux/pid.h>
//#include "ds_monitoring.h"

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

// struct ds_monitoring thread_dm;
// DECLARE_DS_MONITORING(thread_dm);

// struct ds_monitoring thread_dm;
// unsigned long long btree_lookup_time, btree_lookup_count;

// static void print_zone_dm(unsigned long pid,unsigned int cpu, const char *name, unsigned long long count, int percentage){	       	
// 	printk("thread %s: pid %ld called btree_lookup() %lld times (%d%%)\n", name, pid, count, percentage);
// }
		
// static unsigned long get_thread_idx(void *elem){	
// 	int node_idx;
// 	unsigned long xa_index;
// 	int zone_idx;

// 	struct task_struct *current_task = (struct task_struct *) elem;
// 	return (unsigned long) current_task->pid;
// }


// static unsigned int get_cpu_name(void *elem){
// 	struct task_struct *current_task = (struct task_struct *) elem;
// 	return current_task->cpu;
// }

// static const char * get_thread_name(void *elem){
// 	struct task_struct *current_task = (struct task_struct *) elem;
// 	return current_task->comm;
// }

// DEFINE_DS_MONITORING(thread_dm, get_thread_idx, get_cpu_name, get_thread_name, print_zone_dm); 


void insert_element(unsigned long key){

	struct data_element insert_data = {.key = key};

	btree_insert(&tree, &btree_geo32, &key, &insert_data, GFP_KERNEL);

	printk("Inserted key %ld in B+ tree\n", key);
}

struct data_element* find_element(unsigned long key){

	struct data_element *result = btree_lookup(&tree, &btree_geo32, &key);
	// find_ds_monitoring(&thread_dm, current);

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

	// print_ds_monitoring(&thread_dm);
	// delete_ds_monitoring(&thread_dm);
	btree_destroy(&tree);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Herman Bale");
MODULE_DESCRIPTION("A module to create a B+ tree with the included bplus datastructure in the Linux source code");

module_init(bplus_module_init);
module_exit(bplus_module_exit);
