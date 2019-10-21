PWD := $(shell pwd)

ccflags-y += -I$(src)/include/ -g -mpreferred-stack-boundary=4

# Kernel module.
obj-m 	 := dm_template.o

default:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

load:
	@make
	@sudo insmod dm_template.ko

unload:
	@sudo rmmod dm_template
	@make clean

reload:
	@make unload || true
	@make load || true

######################################################################
# Make sure your VM has another disk and its mounted at /dev/sdb.
#
# MY CONFIGURATION: /dev/sdb is 4GB.
# Sectors:
#	196608  = 096MB Artifice instance.
#	524288  = 256MB Artifice instance.
#   1048576 = 512MB Artifice instance.

debug_create:
	@sudo insmod dm_template.ko afs_debug_mode=1
	@echo 0 1048576 dmtemplate /dev/sdb1 | sudo dmsetup create dmtemplate

debug_write:
	sudo dd if=/dev/zero of=/dev/mapper/dmtemplate bs=4096 count=1 oflag=direct

debug_read:
	touch test_output
	sudo dd if=/dev/mapper/dmtemplate of=test_output bs=4096 count=1 oflag=direct

debug_end:
	@sudo dmsetup remove dmtemplate || true
	@sudo rmmod dm_template || true
######################################################################
