obj-m := simple.o
simple-objs=simplefs.o

KERNEL_DIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: ko mkfs-simplefs

ko:
	make -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules

mkfs-simplefs_SOURCES:
	mkfs-simplefs.c simple.h

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
	rm -rf mkfs-simplefs

.PYTHON:clean
