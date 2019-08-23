obj-m := char_driver1.o \
	char_driver2.o \
	char_driver3.o \
	char_driver4.o \
	char_driver5.o \
	char_driver6.o \
	char_driver7.o \
	char_driver8.o \
	char_driver9.o
KERNEL_DIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
all:
	make -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean

.PYTHON:clean
