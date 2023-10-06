obj-m += nic.o

nic-objs := nic_main.o nic_ethtool.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
insmod:
	sudo insmod nic.ko
rmmod:
	sudo rmmod nic.ko