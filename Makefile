obj-m += btree_module.o 
btree_module-y := btree_profiling.o calclock.o #ds_monitoring.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
