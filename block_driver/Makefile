obj-m := blk_driver1.o \
		 blk_driver2.o
KERNEL_DIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
all:
	make -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules
clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean

.PYTHON:clean
