#include "nic.h"


#define NO_PCI

#define PCI_VENDOR_ID_MY 0x11cc

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lc");
MODULE_DESCRIPTION("NIC driver module.");
MODULE_VERSION("0.01");

char nic_driver_name[] = "nic";

static const struct pci_device_id nic_pci_tbl[] = {
    {PCI_DEVICE(PCI_VENDOR_ID_MY, 0x1234)},
    /* required last entry */
    {
        0,
    }};

static int nic_init_module(void);
static void nic_exit_module(void);
static int nic_probe(struct pci_dev *pdev, const struct pci_device_id *ent);
static void nic_remove(struct pci_dev *pdev);
static int __maybe_unused nic_suspend(struct device *dev);
static int __maybe_unused nic_resume(struct device *dev);
static void nic_shutdown(struct pci_dev *pdev);
static pci_ers_result_t nic_io_error_detected(struct pci_dev *pdev,
                                              pci_channel_state_t state);
static pci_ers_result_t nic_io_slot_reset(struct pci_dev *pdev);
static void nic_io_resume(struct pci_dev *pdev);

static const struct pci_error_handlers nic_err_handler = {
    .error_detected = nic_io_error_detected,
    .slot_reset = nic_io_slot_reset,
    .resume = nic_io_resume,
};

static SIMPLE_DEV_PM_OPS(nic_pm_ops, nic_suspend, nic_resume);

static struct pci_driver nic_driver = {.name = nic_driver_name,
                                       .id_table = nic_pci_tbl,
                                       .probe = nic_probe,
                                       .remove = nic_remove,
                                       .driver =
                                           {
                                               .pm = &nic_pm_ops,
                                           },
                                       .shutdown = nic_shutdown,
                                       .err_handler = &nic_err_handler};

int nic_open(struct net_device *netdev);
int nic_close(struct net_device *netdev);
static netdev_tx_t nic_xmit_frame(struct sk_buff *skb,
                                  struct net_device *netdev);
static void nic_set_rx_mode(struct net_device *netdev);
static int nic_set_mac(struct net_device *netdev, void *p);
static void nic_tx_timeout(struct net_device *dev, unsigned int txqueue);
static int nic_change_mtu(struct net_device *netdev, int new_mtu);
static int nic_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd);
static int nic_vlan_rx_add_vid(struct net_device *netdev, __be16 proto,
                               u16 vid);
static int nic_vlan_rx_kill_vid(struct net_device *netdev, __be16 proto,
                                u16 vid);
static void nic_netpoll(struct net_device *netdev);
static netdev_features_t nic_fix_features(struct net_device *netdev,
                                          netdev_features_t features);
static int nic_set_features(struct net_device *netdev,
                            netdev_features_t features);

static const struct net_device_ops nic_netdev_ops = {
    .ndo_open = nic_open,
    .ndo_stop = nic_close,
    .ndo_start_xmit = nic_xmit_frame,
    .ndo_set_rx_mode = nic_set_rx_mode,
    .ndo_set_mac_address = nic_set_mac,
    .ndo_tx_timeout = nic_tx_timeout,
    .ndo_change_mtu = nic_change_mtu,
    .ndo_eth_ioctl = nic_ioctl,
    .ndo_validate_addr = eth_validate_addr,
    .ndo_vlan_rx_add_vid = nic_vlan_rx_add_vid,
    .ndo_vlan_rx_kill_vid = nic_vlan_rx_kill_vid,
#ifdef CONFIG_NET_POLL_CONTROLLER
    .ndo_poll_controller = nic_netpoll,
#endif
    .ndo_fix_features = nic_fix_features,
    .ndo_set_features = nic_set_features,
};

#ifdef NO_PCI

static struct net_device *test_drvdata[4];

#endif

static int __init nic_init_module(void) {
  int ret;
  printk(KERN_INFO "nic_init_module\n");

#ifndef NO_PCI
  ret = pci_register_driver(&nic_driver);
#else
  ret = nic_probe(NULL, NULL);
#endif

  return ret;
}

module_init(nic_init_module);

static void __exit nic_exit_module(void) {
  printk(KERN_INFO "nic_exit_module\n");
#ifndef NO_PCI
  pci_unregister_driver(&nic_driver);
#else
  nic_remove(NULL);
#endif
}

module_exit(nic_exit_module);

static int nic_probe(struct pci_dev *pdev, const struct pci_device_id *ent) {
  struct net_device *netdevs[2];
  int err = 0;

  printk(KERN_INFO "nic_probe\n");

  // net device
  for (int i = 0; i < 2; i++) {
    struct nic_adapter *adapter = netdev_priv(netdevs[i]);
    netdevs[i] = alloc_etherdev(sizeof(struct nic_adapter));
    if (!netdevs[i]) {
      printk(KERN_ERR "alloc_etherdev %u failed\n", i);
      err = -ENOMEM;
      goto err_alloc_etherdev;
    }

    adapter->netdev = netdevs[i];
#ifndef NO_PCI
    adapter->pdev = pdev;
#else
    adapter->pdev = NULL;
#endif
  }

  // io

  // dma

  // ops
  for (int i = 0; i < 2; i++) {
    netdevs[i]->netdev_ops = &nic_netdev_ops;
    nic_set_ethtool_ops(netdevs[i]);
  }

#ifndef NO_PCI
  SET_NETDEV_DEV(netdev, &pdev->dev);
  pci_set_drvdata(pdev, netdev);
#else
  memmove(test_drvdata, netdevs, sizeof(netdevs));
#endif

  return 0;

err_alloc_etherdev:

  return err;
}

static void nic_remove(struct pci_dev *pdev) {
  struct net_device *netdevs[2];
#ifndef NO_PCI
  netdev = pci_get_drvdata(pdev);
#else
  memmove(netdevs, test_drvdata, sizeof(netdevs));
#endif
  printk(KERN_INFO "nic_remove\n");

  // net device
  for (int i = 0; i < 2; i++) {
    // unregister_netdev(netdevs[i]);
    free_netdev(netdevs[i]);
  }
}

static int __maybe_unused nic_suspend(struct device *dev) { return 0; }

static int __maybe_unused nic_resume(struct device *dev) { return 0; }

static void nic_shutdown(struct pci_dev *pdev) {}

static pci_ers_result_t nic_io_error_detected(struct pci_dev *pdev,
                                              pci_channel_state_t state) {
  return PCI_ERS_RESULT_RECOVERED;
}

static pci_ers_result_t nic_io_slot_reset(struct pci_dev *pdev) {
  return PCI_ERS_RESULT_RECOVERED;
}

static void nic_io_resume(struct pci_dev *pdev) {}

// net device

int nic_open(struct net_device *netdev) {
  printk(KERN_INFO "nic_open\n");
  return -EBUSY;
}

int nic_close(struct net_device *netdev) {
  printk(KERN_INFO "nic_close\n");
  return 0;
}

static netdev_tx_t nic_xmit_frame(struct sk_buff *skb,
                                  struct net_device *netdev) {
  return NETDEV_TX_OK;
}

static void nic_set_rx_mode(struct net_device *netdev) {}

static int nic_set_mac(struct net_device *netdev, void *p) { return 0; }

static void nic_tx_timeout(struct net_device *dev, unsigned int txqueue) {}

static int nic_change_mtu(struct net_device *netdev, int new_mtu) { return 0; }

static int nic_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd) {
  return 0;
}

static int nic_vlan_rx_add_vid(struct net_device *netdev, __be16 proto,
                               u16 vid) {
  return 0;
}

static int nic_vlan_rx_kill_vid(struct net_device *netdev, __be16 proto,
                                u16 vid) {
  return 0;
}

static void nic_netpoll(struct net_device *netdev) {}

static netdev_features_t nic_fix_features(struct net_device *netdev,
                                          netdev_features_t features) {
  /* Since there is no support for separate Rx/Tx vlan accel
   * enable/disable make sure Tx flag is always in same state as Rx.
   */
  if (features & NETIF_F_HW_VLAN_CTAG_RX)
    features |= NETIF_F_HW_VLAN_CTAG_TX;
  else
    features &= ~NETIF_F_HW_VLAN_CTAG_TX;

  return features;
}

static int nic_set_features(struct net_device *netdev,
                            netdev_features_t features) {
  // struct nic_adapter *adapter = netdev_priv(netdev);
  // netdev_features_t changed = features ^ netdev->features;

  // do nothing

  return 0;
}
