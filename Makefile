obj-m += cbtree.o 
cbtree-y := btree_profiling.o cbtree_cache.o cbtree_test.o cbtree_base.o calclock.o #ds_monitoring.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean