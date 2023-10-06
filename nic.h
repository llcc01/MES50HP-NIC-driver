#ifndef _NIC_H_
#define _NIC_H_

#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/pci.h>
#include <linux/spinlock_types.h>
#include <linux/stddef.h>
#include <linux/types.h>

#define PCI_VENDOR_ID_MY 0x11cc
#define NIC_DRIVER_NAME "nic"
#define IF_NUM 2
#define NIC_RX_PKT_SIZE 2048

#define PRINT_INFO(fmt, ...)                                                   \
  printk(KERN_INFO NIC_DRIVER_NAME fmt, ##__VA_ARGS__)
#define PRINT_ERR(fmt, ...) printk(KERN_ERR NIC_DRIVER_NAME fmt, ##__VA_ARGS__)

struct nic_tx_desc {
  void *data_va;
  u64 data_pa;
  u16 data_len;
};

struct nic_rx_frame {
  u16 data_len;
  u8 data[NIC_RX_PKT_SIZE - 2];
};

struct nic_tx_ring {
  void *desc_va;
  dma_addr_t desc_pa;

  u16 size;
  u16 count;

  u16 head;
  u16 tail;
};

struct nic_rx_ring {
  void *va;
  dma_addr_t pa;

  u16 size;
  u16 count;

  u16 head;
  u16 tail;
};

struct nic_adapter {
  /* OS defined structs */
  struct net_device *netdev;
  struct pci_dev *pdev;

  /* TX */
  struct nic_tx_ring tx_ring;

  /* RX */
  struct nic_rx_ring rx_ring;
};

void nic_set_ethtool_ops(struct net_device *netdev);

#endif