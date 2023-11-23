obj-m += bplus_module.o
bplus_module-y := ds_monitoring.o bplus_init.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
