#ifndef _NIC_H_
#define _NIC_H_

#include <linux/cdev.h>
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

#include "common.h"

// #define NO_PCI

#ifndef NO_PCI

// #define PCI_FN_TEST

#define NO_INT

#ifdef NO_INT
#define NIC_EMU_INT_JIFFIES (HZ / 100)
#endif

#define NIC_UIO_RX_TIMEOUT_JIFFIES (HZ / 10)

#endif

#define PCI_VENDOR_ID_MY 0x0813

#define PRINT_INFO(fmt, ...)                                                   \
  printk(KERN_INFO NIC_DRIVER_NAME ": " fmt, ##__VA_ARGS__)

#define PRINT_ERR(fmt, ...)                                                    \
  printk(KERN_ERR NIC_DRIVER_NAME ": " fmt, ##__VA_ARGS__)

#define PRINT_WARN(fmt, ...)                                                   \
  printk(KERN_WARNING NIC_DRIVER_NAME ": " fmt, ##__VA_ARGS__)

struct nic_tx_ring {
  union {
    struct sk_buff **skbs;
    void **data_vas;
  };
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

#ifdef NO_INT
  // emulated interrupt
  bool emu_int_rx_enabled;
  bool emu_int_tx_enabled;
#endif

  // uio
  bool uio_enabled;
  struct semaphore raw_sema;
  struct semaphore raw_rx_sema;
  struct semaphore raw_rx_wait_sema;
  struct nic_uio_rx_buf *uio_rx_buf;
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

struct uio_poll_work_ctx {
  struct work_struct work;
  struct nic_adapter *adapter;
};

struct nic_drvdata {
  struct cdev c_dev;
  dev_t c_dev_no;
  struct class *c_dev_class;
  struct net_device *netdevs[NIC_IF_NUM];
};

struct nic_uio_rx_buf {
  void *buf;
  frame_len_t len;
};

struct nic_uio_tx_buf {
  const char __user *buf;
  frame_len_t len;
};

void nic_uio_xmit_frame(struct nic_adapter *adapter,
                        struct nic_uio_tx_buf *uio_tx_buf);

void nic_set_ethtool_ops(struct net_device *netdev);

#endif
