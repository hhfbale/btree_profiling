#include "cbtree.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/random.h>
#include "btree_cache.h" 

#define MAX_OPERATIONS 100000
#define KEY_RANGE 10000

struct btree_head tree;

static int __init btree_test_init(void) {
    unsigned long key;
    void *ret;
    int i;

    printk(KERN_INFO "Starting B-Tree Test Module\n");

    btree_init(&tree);

    for (i = 0; i < MAX_OPERATIONS; i++) {
        get_random_bytes(&key, sizeof(key));
        key %= KEY_RANGE;

        btree_insert(&tree, &key, (void *)key, GFP_KERNEL);
        setcache(&cache, &tree, &key, &key, sizeof(key), sizeof(key));

        ret = findNode(&cache, &key, &tree);
        if (ret) {
            printk(KERN_INFO "Found key: %lu\n", key);
        } else {
            printk(KERN_INFO "Key not found: %lu\n", key);
        }

        btree_remove(&tree, &key);
    }

    return 0;
}

static void __exit btree_test_exit(void) {
    printk(KERN_INFO "Exiting B-Tree Test Module\n");
    freeQueue(&cache, &tree, sizeof(unsigned long));
    btree_destroy(&tree);
}

module_init(btree_test_init);
module_exit(btree_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("B-Tree Test Kernel Module");
