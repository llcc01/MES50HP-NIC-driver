obj-m += nic.o

nic-objs := nic_main.o nic_ethtool.o nic_cdev.o nic_hw.o

.PHONY: all
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	make -C $(PWD)/app

.PHONY: clean
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	make -C $(PWD)/app clean

.PHONY: insmod
mod:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

.PHONY: app
app:
	make -C $(PWD)/app

.PHONY: set_hw
set_hw:app
	sudo ./app/app set_hw

.PHONY: poll
poll:app
	sudo ./app/app poll

.PHONY: rmmod
insmod:
	sudo insmod nic.ko

.PHONY: rmmod
rmmod:
	sudo rmmod nic.ko

.PHONY: ip
ip:
	sudo ip a add 192.168.10.1/24 dev eth0
	sudo ip a add 192.168.11.1/24 dev eth1
	ip a

.PHONY: ip2
ip2:
	sudo ip a add 192.168.10.1/24 dev enp0s1
	sudo ip a add 192.168.11.1/24 dev eth1
	ip a

.PHONY: ip3
ip3:
	sudo ip a add 192.168.10.1/24 dev eth0
	sudo ip a add 192.168.11.1/24 dev enp0s1
	ip a

.PHONY: linkup
linkup:
	sudo ip link set eth0 up
	sudo ip link set eth1 up

.PHONY: linkup2
linkup2:
	sudo ip link set enp0s1 up
	sudo ip link set eth1 up

.PHONY: linkup3
linkup3:
	sudo ip link set eth0 up
	sudo ip link set enp0s1 up

.PHONY: route
route:
	sudo ip route add 192.168.20.0/24 via 192.168.0.1