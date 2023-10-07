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
ip:
	sudo ip a add 192.168.0.1/24 dev eth0
	sudo ip a add 192.168.1.1/24 dev eth1
	ip a
route:
	sudo ip route add 192.168.10.0/24 via 192.168.0.1