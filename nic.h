#ifndef _NIC_H_
#define _NIC_H_

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/spinlock_types.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/pci.h>
#include <linux/ethtool.h>

struct nic_adapter {
  /* OS defined structs */
  struct net_device *netdev;
  struct pci_dev *pdev;
};

void nic_set_ethtool_ops(struct net_device *netdev);

#endif