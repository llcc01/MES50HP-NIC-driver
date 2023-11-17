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

.PHONY: listen0
listen0:app
	sudo ./app/app listen 0

.PHONY: send1
send1:app
	sudo ./app/app send 1

.PHONY: uio_en0
uio_en0:app
	sudo ./app/app uio_en 0

.PHONY: uio_en1
uio_en1:app
	sudo ./app/app uio_en 1

.PHONY: uio_dis0
uio_dis0:app
	sudo ./app/app uio_dis 0

.PHONY: uio_dis1
uio_dis1:app
	sudo ./app/app uio_dis 1

.PHONY: insmod
insmod:
	sudo insmod nic.ko

.PHONY: reload
reload:
	sudo rmmod nic.ko
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
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

.PHONY: ip4
ip4:
	sudo ip a add 192.168.10.1/24 dev eth0
	sudo ip a add 192.168.11.1/24 dev enp1s0
	ip a

.PHONY: ip_del4
ip_del4:
	sudo ip a del 192.168.10.1 dev eth0
	sudo ip a del 192.168.11.1 dev enp1s0

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
