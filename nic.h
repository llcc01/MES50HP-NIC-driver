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

// #define NO_PCI

#ifndef NO_PCI

// #define PCI_FN_TEST

#endif

#define PCI_VENDOR_ID_MY 0x0755

#define NIC_DRIVER_NAME "nic"

#define NIC_IF_NUM 2

#define NIC_RX_PKT_SIZE 2048

#define NIC_TX_RING_QUEUES 128

#define NIC_RX_RING_QUEUES 128

// mmio

#define NIC_IF_REG_SIZE BIT(9)

#define NIC_PCIE_CTL_CALC_OFFSET(s, offset) (((s) << 5) + ((offset) << 2))

// tx

#define NIC_DMA_CTL_TX_BD_BA_LOW NIC_PCIE_CTL_CALC_OFFSET(0x0, 0x0)

#define NIC_DMA_CTL_TX_BD_BA_HIGH NIC_PCIE_CTL_CALC_OFFSET(0x0, 0x1)

#define NIC_DMA_CTL_TX_BD_SIZE NIC_PCIE_CTL_CALC_OFFSET(0x0, 0x2)

// #define NIC_DMA_CTL_TX_BD_HEAD NIC_PCIE_CTL_CALC_OFFSET(0x0, 0x3)

#define NIC_DMA_CTL_TX_BD_TAIL NIC_PCIE_CTL_CALC_OFFSET(0x0, 0x4)

// rx

#define NIC_DMA_CTL_RX_BD_BA_LOW NIC_PCIE_CTL_CALC_OFFSET(0x1, 0x0)

#define NIC_DMA_CTL_RX_BD_BA_HIGH NIC_PCIE_CTL_CALC_OFFSET(0x1, 0x1)

#define NIC_DMA_CTL_RX_BD_SIZE NIC_PCIE_CTL_CALC_OFFSET(0x1, 0x2)

// #define NIC_DMA_CTL_RX_BD_HEAD NIC_PCIE_CTL_CALC_OFFSET(0x1, 0x3)

#define NIC_DMA_CTL_RX_BD_TAIL NIC_PCIE_CTL_CALC_OFFSET(0x1, 0x4)

// interrupt

#define NIC_CSR_CTL_INT_OFFSET(tx_rx) NIC_PCIE_CTL_CALC_OFFSET(0x2, (tx_rx))

// vector

#define NIC_VEC_TX 0

#define NIC_VEC_RX 1

#define NIC_VEC_OTHER 2

#define NIC_VEC_IF_SIZE 4

#define PRINT_INFO(fmt, ...)                                                   \
  printk(KERN_INFO NIC_DRIVER_NAME ": " fmt, ##__VA_ARGS__)

#define PRINT_ERR(fmt, ...)                                                    \
  printk(KERN_ERR NIC_DRIVER_NAME ": " fmt, ##__VA_ARGS__)

#define PRINT_WARN(fmt, ...)                                                   \
  printk(KERN_WARNING NIC_DRIVER_NAME ": " fmt, ##__VA_ARGS__)

#define NIC_BD_FLAG_VALID BIT(0)
#define NIC_BD_FLAG_USED BIT(1)

struct nic_bd {
  dma_addr_t addr;
  u32 len; // for tx, len is the length of the packet
  u32 flags;
};

struct nic_rx_frame {
  u8 data[NIC_RX_PKT_SIZE];
};

struct nic_tx_ring {
  struct sk_buff **skbs;
  struct nic_bd *bd_va;
  dma_addr_t bd_pa;

  u16 bd_size;
  u16 bd_dma_size;

  u16 next_to_use;
  u16 next_to_clean;
};

struct nic_rx_ring {
  void **data_vas;
  struct nic_bd *bd_va;
  dma_addr_t bd_pa;

  u16 bd_size;
  u16 bd_dma_size;

  u16 next_to_use;
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

  u16 if_id;

  int bars;

  void *io_addr;
  unsigned long io_base;
  u64 io_size;
  int irq_tx;
  int irq_rx;
  int irq_other;
};

#ifdef NO_PCI

struct test_work_ctx {
  struct work_struct work;
  u16 dst;
  u16 src;
};

#endif

struct clean_work_ctx {
  struct work_struct work;
  struct nic_adapter *adapter;
};

void nic_set_ethtool_ops(struct net_device *netdev);

#endif
