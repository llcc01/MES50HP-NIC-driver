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

#define NO_PCI

#define PCI_VENDOR_ID_MY 0x11cc
#define NIC_DRIVER_NAME "nic"
#define IF_NUM 2
#define NIC_RX_PKT_SIZE 2048
#define NIC_TX_RING_COUNT 16
#define NIC_RX_RING_COUNT 16

#define NIC_BAR_TX_RING_HEAD 0x00
#define NIC_BAR_TX_RING_TAIL 0x04
#define NIC_BAR_TX_RING_HEAD_PA 0x08 // reserved
#define NIC_BAR_TX_RING_TAIL_PA 0x10 // reserved
#define NIC_BAR_TX_RING_DESC_PA 0x18

#define NIC_BAR_RX_RING_HEAD 0x20 // reserved
#define NIC_BAR_RX_RING_TAIL 0x24 // reserved
#define NIC_BAR_RX_RING_HEAD_PA 0x28
#define NIC_BAR_RX_RING_TAIL_PA 0x30
#define NIC_BAR_RX_RING_PA 0x38

#define PRINT_INFO(fmt, ...)                                                   \
  printk(KERN_INFO NIC_DRIVER_NAME ": " fmt, ##__VA_ARGS__)
#define PRINT_ERR(fmt, ...)                                                    \
  printk(KERN_ERR NIC_DRIVER_NAME ": " fmt, ##__VA_ARGS__)

struct nic_tx_desc {
  void *data_va;
  dma_addr_t data_pa;
  u16 data_len;
};

struct nic_rx_frame {
  u16 data_len;
  u8 data[NIC_RX_PKT_SIZE - 2];
};

struct nic_tx_ring {
  struct nic_tx_desc *desc_va;
  dma_addr_t desc_pa;

  u16 size;
  u16 count;

  u16 head;
  u16 tail; // reserved, for test
};

struct nic_rx_ring {
  void *va;
  dma_addr_t pa;

  u16 size;
  u16 count;

  u16 *head_va;
  dma_addr_t head_pa;

  u16 tail;
};

struct nic_adapter {
  /* OS defined structs */
  struct net_device *netdev;
  struct pci_dev *pdev;

  int msg_enable;

  /* TX */
  struct nic_tx_ring tx_ring;

  /* RX */
  struct nic_rx_ring rx_ring;
  struct napi_struct napi;

  u64 hw_addr;
  u16 if_id;
};

#ifdef NO_PCI

struct test_work_data {
  struct nic_adapter *dst;
  struct nic_adapter *src;
};

#endif

void nic_set_ethtool_ops(struct net_device *netdev);

#endif