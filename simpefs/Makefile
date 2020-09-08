obj-m := simplefs.o 
simplefs-objs=simple.o
KERNEL_DIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
all: ko mkfs-simplefs
ko:
	make -C $(KERNEL_DIR) M=$(PWD) modules
mkfs-simplefs_SOURCES:
	mkfs-simplefs.c simple.h
clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
	rm mkfs-simplefs -f

.PYTHON:clean
