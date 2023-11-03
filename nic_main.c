#include "nic.h"
#include "nic_cdev.h"
#include "nic_hw.h"
#include <linux/dma-mapping.h>
#include <linux/timer.h>
#include <linux/version.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lc");
MODULE_DESCRIPTION("NIC driver module.");
MODULE_VERSION("0.01");

char nic_driver_name[] = NIC_DRIVER_NAME;

static const struct pci_device_id nic_pci_tbl[] = {
    {PCI_DEVICE(PCI_VENDOR_ID_MY, 0x0755)},
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
#ifndef NO_PCI
static void nic_shutdown(struct pci_dev *pdev);
#endif
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

#ifndef NO_PCI
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
#endif

#ifndef PCI_FN_TEST

int nic_setup_all_resources(struct nic_adapter *adapter);
void nic_free_all_resources(struct nic_adapter *adapter);

int nic_open(struct net_device *netdev);
int nic_close(struct net_device *netdev);
static netdev_tx_t nic_xmit_frame(struct sk_buff *skb,
                                  struct net_device *netdev);
static struct sk_buff *nic_receive_skb(struct nic_adapter *adapter);
static void nic_set_rx_mode(struct net_device *netdev);
static int nic_set_mac(struct net_device *netdev, void *p);
static void nic_tx_timeout(struct net_device *dev, unsigned int txqueue);
static int nic_change_mtu(struct net_device *netdev, int new_mtu);
static int nic_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd);
// static int nic_vlan_rx_add_vid(struct net_device *netdev, __be16 proto,
//                                u16 vid);
// static int nic_vlan_rx_kill_vid(struct net_device *netdev, __be16 proto,
//                                 u16 vid);
static void nic_netpoll(struct net_device *netdev);
static netdev_features_t nic_fix_features(struct net_device *netdev,
                                          netdev_features_t features);
static int nic_set_features(struct net_device *netdev,
                            netdev_features_t features);

static int nic_poll(struct napi_struct *napi, int budget);
#ifndef NO_PCI
static void nic_set_int(struct nic_adapter *adapter, int nr, bool enable);
static irqreturn_t nic_interrupt_tx(int irq, void *data);
static irqreturn_t nic_interrupt_rx(int irq, void *data);
static void nic_clean_tx_ring_work(struct work_struct *work);
#endif

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
// .ndo_vlan_rx_add_vid = nic_vlan_rx_add_vid,
// .ndo_vlan_rx_kill_vid = nic_vlan_rx_kill_vid,
#ifdef CONFIG_NET_POLL_CONTROLLER
    .ndo_poll_controller = nic_netpoll,
#endif
    .ndo_fix_features = nic_fix_features,
    .ndo_set_features = nic_set_features,
};
#endif // PCI_FN_TEST
#ifdef NO_PCI
static struct net_device *test_netdev[NIC_IF_NUM];
#endif
static struct workqueue_struct *nic_wq;

static int __init nic_init_module(void) {
  int ret;
  PRINT_INFO("nic_init_module\n");

#ifndef NO_PCI
  ret = pci_register_driver(&nic_driver);
#else
  ret = nic_probe(NULL, NULL);
#endif
  nic_wq = create_singlethread_workqueue("nic_wq");

  return ret;
}

module_init(nic_init_module);

static void __exit nic_exit_module(void) {
  PRINT_INFO("nic_exit_module\n");
#ifndef NO_PCI
  pci_unregister_driver(&nic_driver);
#else
  nic_remove(NULL);
#endif
  destroy_workqueue(nic_wq);
}

module_exit(nic_exit_module);

#ifndef PCI_FN_TEST
static int nic_probe(struct pci_dev *pdev, const struct pci_device_id *ent) {
  struct nic_drvdata *drvdata;
  struct nic_adapter *adapter[NIC_IF_NUM];
  int err = 0;
  size_t i;
#ifndef NO_PCI
  int bars;
#endif

  PRINT_INFO("nic_probe\n");

  // pcie
#ifndef NO_PCI
  err = pci_enable_device_mem(pdev);
  if (err) {
    PRINT_ERR("pci_enable_device failed\n");
    return err;
  }

  bars = pci_select_bars(pdev, IORESOURCE_MEM);
  err = pci_request_selected_regions(pdev, bars, nic_driver_name);
  if (err) {
    PRINT_ERR("pci_request_selected_regions failed\n");
    goto err_request_mem_regions;
  }

  pci_set_master(pdev);
  err = pci_save_state(pdev);
  if (err) {
    PRINT_ERR("pci_save_state failed\n");
    goto err_alloc_drvdata;
  }
  PRINT_INFO("pci_enable_device\n");
#endif

  drvdata = kmalloc(sizeof(struct nic_drvdata), GFP_KERNEL);
  // net device
  if (!drvdata) {
    PRINT_ERR("alloc drvdata failed\n");
    err = -ENOMEM;
    goto err_alloc_drvdata;
  }

  for (i = 0; i < NIC_IF_NUM; i++) {
    drvdata->netdevs[i] = alloc_etherdev(sizeof(struct nic_adapter));
    adapter[i] = netdev_priv(drvdata->netdevs[i]);
    if (!drvdata->netdevs[i]) {
      PRINT_ERR("alloc_etherdev %zu failed\n", i);
      err = -ENOMEM;
      goto err_alloc_etherdev;
    }

    adapter[i]->netdev = drvdata->netdevs[i];
    adapter[i]->if_id = i;
#ifndef NO_PCI
    adapter[i]->pdev = pdev;
    adapter[i]->bars = bars;
#else
    adapter[i]->pdev = NULL;
#endif
  }

#ifndef NO_PCI
  for (i = 0; i < NIC_IF_NUM; i++) {
    SET_NETDEV_DEV(drvdata->netdevs[i], &pdev->dev);
  }
  pci_set_drvdata(pdev, drvdata);
#else
  memmove(test_netdev, netdevs, sizeof(void *) * NIC_IF_NUM);
#endif
  PRINT_INFO("alloc netdev\n");

#ifndef NO_PCI
  // ioremap
  adapter[0]->io_size = pci_resource_len(pdev, 0);
  adapter[0]->io_base = pci_resource_start(pdev, 0);
  adapter[0]->io_addr = pci_ioremap_bar(pdev, 0);
  if (!adapter[0]->io_addr) {
    PRINT_ERR("pci_ioremap_bar failed\n");
    err = -ENOMEM;
    goto err_ioremap;
  }
  for (i = 1; i < NIC_IF_NUM; i++) {
    adapter[i]->io_size = adapter[0]->io_size;
    adapter[i]->io_base = adapter[0]->io_base;
    adapter[i]->io_addr = adapter[i - 1]->io_addr + NIC_IF_REG_SIZE;
  }
  PRINT_INFO("pci_ioremap\n");

  // dma
  err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
  if (err) {
    PRINT_ERR("No usable DMA config, aborting\n");
    goto err_dma;
  }
  PRINT_INFO("pci_set_dma_mask\n");
#endif

  // ops
  for (i = 0; i < NIC_IF_NUM; i++) {
    // char mac_addr[ETH_ALEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    drvdata->netdevs[i]->netdev_ops = &nic_netdev_ops;
    nic_set_ethtool_ops(drvdata->netdevs[i]);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
    netif_napi_add(drvdata->netdevs[i], &adapter[i]->napi, nic_poll);
#else
    netif_napi_add(drvdata->netdevs[i], &adapter[i]->napi, nic_poll, NAPI_POLL_WEIGHT);
#endif
    // eth_hw_addr_set(netdev[i], adapter[i]->mac_addr);
    eth_hw_addr_random(drvdata->netdevs[i]);
  }
  PRINT_INFO("set ops\n");

  for (i = 0; i < NIC_IF_NUM; i++) {
    err = register_netdev(drvdata->netdevs[i]);
    if (err) {
      PRINT_ERR("register_netdev %zu failed\n", i);
      goto err_register;
    }
    netif_carrier_off(drvdata->netdevs[i]);
  }
  PRINT_INFO("register netdev\n");

#ifndef NO_PCI
  // irq
  err = pci_alloc_irq_vectors(pdev, NIC_VEC_IF_SIZE * NIC_IF_NUM,
                              NIC_VEC_IF_SIZE * NIC_IF_NUM, PCI_IRQ_MSI);
  if (err < 0) {
    PRINT_ERR("pci_alloc_irq_vectors failed\n");
    goto err_alloc_irq_vectors;
  }
  PRINT_INFO("pci_alloc_irq_vectors\n");

  for (i = 0; i < NIC_IF_NUM; i++) {
    adapter[i]->irq_tx = pci_irq_vector(pdev, NIC_VEC_IF_SIZE * i + NIC_VEC_TX);
    err = request_irq(adapter[i]->irq_tx, nic_interrupt_tx, 0, nic_driver_name,
                      drvdata->netdevs[i]);
    if (err) {
      PRINT_ERR("request_irq %zu nic_interrupt_tx failed\n", i);
      goto err_request_irq;
    }

    adapter[i]->irq_rx = pci_irq_vector(pdev, NIC_VEC_IF_SIZE * i + NIC_VEC_RX);
    err = request_irq(adapter[i]->irq_rx, nic_interrupt_rx, 0, nic_driver_name,
                      drvdata->netdevs[i]);
    if (err) {
      PRINT_ERR("request_irq %zu nic_interrupt_rx failed\n", i);
      goto err_request_irq;
    }

    if (err) {
      PRINT_ERR("request_irq %zu nic_interrupt_other failed\n", i);
      goto err_request_irq;
    }
  }

  // cdev
  err = nic_init_cdev(drvdata);
  if (err) {
    PRINT_ERR("nic_init_cdev failed\n");
    goto err_cdev;
  }

#endif

  PRINT_INFO("nic_probe done\n");
  return 0;

#ifndef NO_PCI
err_cdev:

  for (i = 0; i < NIC_IF_NUM; i++) {
    free_irq(adapter[i]->irq_tx, drvdata->netdevs[i]);
    free_irq(adapter[i]->irq_rx, drvdata->netdevs[i]);
  }
err_request_irq:

  pci_free_irq_vectors(pdev);
err_alloc_irq_vectors:

  for (i = 0; i < NIC_IF_NUM; i++) {
    unregister_netdev(drvdata->netdevs[i]);
  }
#endif
err_register:
#ifndef NO_PCI
err_dma:
err_ioremap:
#endif
err_alloc_etherdev:

  kfree(drvdata);
err_alloc_drvdata:

#ifndef NO_PCI

  pci_release_selected_regions(pdev, bars);
err_request_mem_regions:

  pci_disable_device(pdev);
#endif

  return err;
}

static void nic_remove(struct pci_dev *pdev) {
  struct nic_drvdata *drvdata;
  size_t i;
#ifndef NO_PCI
  struct nic_adapter *adapter[NIC_IF_NUM];
#endif

  PRINT_INFO("nic_remove\n");

#ifndef NO_PCI
  drvdata = pci_get_drvdata(pdev);
  for (i = 0; i < NIC_IF_NUM; i++) {
    adapter[i] = netdev_priv(drvdata->netdevs[i]);
  }
  nic_exit_cdev(drvdata);
#else
  netdevs = test_netdev;
#endif

  // irq
#ifndef NO_PCI
  for (i = 0; i < NIC_IF_NUM; i++) {
    free_irq(adapter[i]->irq_tx, drvdata->netdevs[i]);
    free_irq(adapter[i]->irq_rx, drvdata->netdevs[i]);
  }
  pci_free_irq_vectors(pdev);
  PRINT_INFO("free_irq\n");
#endif

  // net device
  for (i = 0; i < NIC_IF_NUM; i++) {
    unregister_netdev(drvdata->netdevs[i]);
  }
  PRINT_INFO("unregister netdev\n");

  // iounmap
#ifndef NO_PCI
  iounmap(adapter[0]->io_addr);
  pci_release_selected_regions(pdev, adapter[0]->bars);
  PRINT_INFO("iounmap\n");
#endif

  // net device
  for (i = 0; i < NIC_IF_NUM; i++) {
    free_netdev(drvdata->netdevs[i]);
  }

  kfree(drvdata);
#ifndef NO_PCI
  pci_disable_device(pdev);
#endif
}

#else

static u32 test_io_size;
static u32 test_io_base;
static u32 *test_io_addr;
static int test_bars;

struct timer_list test_timer;

void test_timer_func(struct timer_list *t) {
  // PRINT_INFO("test_timer_func\n");
  // test_io_addr[0] = 0x55aa;
  writel(0x55aa, test_io_addr);
  mod_timer(&test_timer, jiffies + 1); // max frequency
}

static int nic_probe(struct pci_dev *pdev, const struct pci_device_id *ent) {
  int err = 0;
  PRINT_INFO("nic_probe\n");

  // pcie
  err = pci_enable_device_mem(pdev);
  if (err) {
    PRINT_ERR("pci_enable_device failed\n");
    return err;
  }

  test_bars = pci_select_bars(pdev, IORESOURCE_MEM);
  err = pci_request_selected_regions(pdev, test_bars, nic_driver_name);
  if (err) {
    PRINT_ERR("pci_request_selected_regions failed\n");
    goto err_request_mem_regions;
  }

  pci_set_master(pdev);
  err = pci_save_state(pdev);
  if (err) {
    PRINT_ERR("pci_save_state failed\n");
    goto err_ioremap;
  }
  PRINT_INFO("pci_enable_device\n");

  // ioremap
  test_io_size = pci_resource_len(pdev, 0);
  test_io_base = pci_resource_start(pdev, 0);
  test_io_addr = pci_ioremap_bar(pdev, 0);
  if (!test_io_addr) {
    PRINT_ERR("pci_ioremap_bar failed\n");
    err = -ENOMEM;
    goto err_ioremap;
  }
  PRINT_INFO("pci_ioremap\n");

  // dma
  err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
  if (err) {
    PRINT_ERR("No usable DMA config, aborting\n");
    goto err_dma;
  }
  PRINT_INFO("pci_set_dma_mask\n");

  test_timer.expires = jiffies + 1 * HZ;
  test_timer.function = test_timer_func;
  add_timer(&test_timer);

  PRINT_INFO("nic_probe done\n");
  return 0;

err_dma:
err_ioremap:
  pci_release_selected_regions(pdev, test_bars);
err_request_mem_regions:
  pci_disable_device(pdev);

  return err;
}

static void nic_remove(struct pci_dev *pdev) {

  PRINT_INFO("nic_remove\n");

  del_timer(&test_timer);

  // iounmap
  iounmap(test_io_addr);
  pci_release_selected_regions(pdev, test_bars);
  PRINT_INFO("iounmap\n");

  pci_disable_device(pdev);
}
#endif

static int __maybe_unused nic_suspend(struct device *dev) { return 0; }

static int __maybe_unused nic_resume(struct device *dev) { return 0; }

#ifndef NO_PCI
static void nic_shutdown(struct pci_dev *pdev) {}
#endif

static pci_ers_result_t nic_io_error_detected(struct pci_dev *pdev,
                                              pci_channel_state_t state) {
  return PCI_ERS_RESULT_RECOVERED;
}

static pci_ers_result_t nic_io_slot_reset(struct pci_dev *pdev) {
  return PCI_ERS_RESULT_RECOVERED;
}

static void nic_io_resume(struct pci_dev *pdev) {}

#ifndef PCI_FN_TEST

// resource management

static int nic_alloc_queues(struct nic_adapter *adapter) {
#ifndef NO_PCI
  struct pci_dev *pdev = adapter->pdev;
#endif
  struct nic_tx_ring *tx_ring = &adapter->tx_ring;
  struct nic_rx_ring *rx_ring = &adapter->rx_ring;
  int err = 0;
  // struct nic_bd *tx_bd_va;
  // dma_addr_t *tx_bd_pa;
  // struct nic_bd *rx_bd_va;
  // dma_addr_t *rx_bd_pa;
  size_t i;
#ifndef NO_PCI
  dma_addr_t rx_buffer_pa;
#endif

  // TX
  tx_ring->bd_size = NIC_TX_RING_QUEUES;

  tx_ring->skbs = kcalloc(tx_ring->bd_size, sizeof(void *), GFP_KERNEL);
  if (!tx_ring->skbs) {
    PRINT_ERR("alloc tx_ring data_vas failed\n");
    err = -ENOMEM;
    goto err_tx;
  }

  // TX BD

#ifndef NO_PCI
  /* round up to nearest 4K */
  tx_ring->bd_dma_size = ALIGN(sizeof(struct nic_bd) * tx_ring->bd_size, 4096);
  tx_ring->bd_va = dma_alloc_coherent(&pdev->dev, tx_ring->bd_dma_size,
                                      &tx_ring->bd_pa, GFP_KERNEL);
  memset(tx_ring->bd_va, 0, sizeof(struct nic_bd) * tx_ring->bd_size);
#else
  tx_ring->bd_va = kcalloc(tx_ring->size, sizeof(struct nic_bd), GFP_KERNEL);
#endif

  if (!tx_ring->bd_va) {
    PRINT_ERR("alloc tx_ring bd failed\n");
    err = -ENOMEM;
    goto err_tx_bd;
  }

  // RX

  rx_ring->bd_size = NIC_RX_RING_QUEUES;

  rx_ring->data_vas = kcalloc(rx_ring->bd_size, sizeof(void *), GFP_KERNEL);

  if (!rx_ring->data_vas) {
    PRINT_ERR("alloc rx_ring vas failed\n");
    err = -ENOMEM;
    goto err_rx;
  }

  // RX buffer
  // contiguous

#ifndef NO_PCI
  rx_ring->data_vas[0] = dma_alloc_coherent(
      &pdev->dev, sizeof(struct nic_rx_frame) * rx_ring->bd_size, &rx_buffer_pa,
      GFP_KERNEL);
#else
  rx_ring->data_vas[0] =
      kcalloc(rx_ring->size, sizeof(struct nic_rx_frame), GFP_KERNEL);
#endif

  if (!rx_ring->data_vas[0]) {
    PRINT_ERR("alloc rx_ring buffer failed\n");
    err = -ENOMEM;
    goto err_rx_buffer;
  }

  for (i = 1; i < rx_ring->bd_size; i++) {
    rx_ring->data_vas[i] =
        rx_ring->data_vas[i - 1] + sizeof(struct nic_rx_frame);
  }

  // RX BD
#ifndef NO_PCI
  /* round up to nearest 4K */
  rx_ring->bd_dma_size = ALIGN(sizeof(struct nic_bd) * rx_ring->bd_size, 4096);

  rx_ring->bd_va = dma_alloc_coherent(&pdev->dev, rx_ring->bd_dma_size,
                                      &rx_ring->bd_pa, GFP_KERNEL);
  for (i = 0; i < rx_ring->bd_size; i++) {
    rx_ring->bd_va[i].addr = rx_buffer_pa + sizeof(struct nic_rx_frame) * i;
  }
#else
  rx_ring->bd_va = kcalloc(rx_ring->size, sizeof(struct nic_bd), GFP_KERNEL);
#endif

  if (!rx_ring->data_vas) {
    PRINT_ERR("dma_alloc_coherent rx_ring bd failed\n");
    err = -ENOMEM;
    goto err_rx_bd;
  }

  // check_64k_bound
  // TODO

#ifndef NO_PCI
  // write reg
  

  // sync_with_hw_tail
  tx_ring->next_to_use = readl(adapter->io_addr + NIC_DMA_CTL_TX_BD_TAIL);
  rx_ring->next_to_use = readl(adapter->io_addr + NIC_DMA_CTL_RX_BD_TAIL);
#endif

  return 0;

err_rx_bd:
#ifndef NO_PCI
  dma_free_coherent(&pdev->dev, sizeof(struct nic_rx_frame) * rx_ring->bd_size,
                    rx_ring->data_vas[0], rx_ring->bd_va[0].addr);
#else
  kfree(rx_ring->data_vas[0]);
#endif

err_rx_buffer:
  kfree(rx_ring->data_vas);

err_rx:
#ifndef NO_PCI
  dma_free_coherent(&pdev->dev, tx_ring->bd_dma_size, tx_ring->bd_va,
                    tx_ring->bd_pa);
#else
  kfree(tx_ring->bd_va);

#endif

err_tx_bd:
  kfree(tx_ring->skbs);

err_tx:
  return err;
}

static int nic_free_queues(struct nic_adapter *adapter) {
  struct nic_tx_ring *tx_ring = &adapter->tx_ring;
  struct nic_rx_ring *rx_ring = &adapter->rx_ring;

#ifndef NO_PCI
  struct pci_dev *pdev = adapter->pdev;

  writel(0, adapter->io_addr + NIC_DMA_CTL_TX_BD_BA_LOW);
  writel(0, adapter->io_addr + NIC_DMA_CTL_TX_BD_BA_HIGH);
  writel(0, adapter->io_addr + NIC_DMA_CTL_RX_BD_BA_LOW);
  writel(0, adapter->io_addr + NIC_DMA_CTL_RX_BD_BA_HIGH);

  dma_free_coherent(&pdev->dev, sizeof(struct nic_rx_frame) * rx_ring->bd_size,
                    rx_ring->data_vas[0], rx_ring->bd_va[0].addr);
  dma_free_coherent(&pdev->dev, rx_ring->bd_dma_size, rx_ring->bd_va,
                    rx_ring->bd_pa);
  kfree(rx_ring->data_vas);

  dma_free_coherent(&pdev->dev, tx_ring->bd_dma_size, tx_ring->bd_va,
                    tx_ring->bd_pa);
  kfree(tx_ring->skbs);
#else
  kfree(rx_ring->data_vas[0]);
  kfree(rx_ring->bd_va);
  kfree(rx_ring->data_vas);

  kfree(tx_ring->bd_va);
  kfree(tx_ring->skbs);
#endif

  tx_ring->bd_size = 0;
  // tx_ring->size = 0;
  rx_ring->bd_size = 0;
  // rx_ring->size = 0;
  return 0;
}

int nic_setup_all_resources(struct nic_adapter *adapter) {
  int err = 0;
  err = nic_alloc_queues(adapter);
  if (err) {
    PRINT_ERR("nic_alloc_queues failed\n");
    goto err_alloc_queues;
  }

  return 0;

err_alloc_queues:
  return err;
}

void nic_free_all_resources(struct nic_adapter *adapter) {
  nic_free_queues(adapter);
}

// net device

int nic_open(struct net_device *netdev) {
  struct nic_adapter *adapter = netdev_priv(netdev);
  int err;
  netdev_info(netdev, "nic_open\n");

  netif_carrier_off(netdev);

  err = nic_setup_all_resources(adapter);
  if (err) {
    goto err_setup;
  }

  napi_enable(&adapter->napi);

#ifndef NO_PCI
  nic_set_int(adapter, NIC_VEC_TX, true);
  nic_set_int(adapter, NIC_VEC_RX, true);
  // nic_set_int(adapter, NIC_VEC_OTHER, true);
#endif

  netif_start_queue(netdev);

  netif_carrier_on(netdev);
  netdev_info(netdev, "netif_carrier_on\n");

  return 0;

err_setup:
  return err;
}

int nic_close(struct net_device *netdev) {
  struct nic_adapter *adapter = netdev_priv(netdev);
  netdev_info(netdev, "nic_close\n");

#ifndef NO_PCI
  nic_set_int(adapter, NIC_VEC_TX, false);
  nic_set_int(adapter, NIC_VEC_RX, false);
  // nic_set_int(adapter, NIC_VEC_OTHER, false);
#endif

  netif_tx_disable(netdev);
  netif_carrier_off(netdev);
  napi_disable(&adapter->napi);

  nic_free_all_resources(adapter);

  return 0;
}

// for test
#ifdef NO_PCI
void print_ring(void) {
  size_t i;
  for (i = 0; i < NIC_IF_NUM; i++) {
    struct nic_adapter *adapter = netdev_priv(test_netdev[i]);
    struct nic_tx_ring *tx_ring = &adapter->tx_ring;
    struct nic_rx_ring *rx_ring = &adapter->rx_ring;

    PRINT_INFO("if_id: %u\n", adapter->if_id);
    PRINT_INFO("tx_ring->size: %u\n", tx_ring->size);
    PRINT_INFO("tx_ring->next_to_use: %u\n", tx_ring->next_to_use);
    PRINT_INFO("tx_ring->next_to_clean: %u\n", tx_ring->next_to_clean);
    PRINT_INFO("rx_ring->size: %u\n", rx_ring->size);
    PRINT_INFO("rx_ring->next_to_use: %u\n", rx_ring->next_to_use);
  }
}

void test_copy_data(struct work_struct *work) {
  struct test_work_ctx *work_ctx =
      container_of(work, struct test_work_ctx, work);

  struct net_device *netdev_dst = test_netdev[work_ctx->dst];
  struct net_device *netdev_src = test_netdev[work_ctx->src];

  struct nic_adapter *dst = netdev_priv(netdev_dst);
  struct nic_adapter *src = netdev_priv(netdev_src);

  struct nic_tx_ring *src_tx_ring = &src->tx_ring;
  struct nic_rx_ring *dst_rx_ring = &dst->rx_ring;

  struct sk_buff *skb;

  PRINT_INFO("test_copy_data called\n");

  while (src_tx_ring->bd_va[src_tx_ring->next_to_use].flags &
         NIC_BD_FLAG_USED) {
    struct nic_bd *bd = &src_tx_ring->bd_va[src_tx_ring->next_to_use];
    struct nic_rx_frame *frame =
        dst_rx_ring->data_vas[dst_rx_ring->next_to_use];

    PRINT_INFO("test_copy_data %u->%u: %u\n", src->if_id, dst->if_id, bd->len);

    if (!frame) {
      netdev_err(netdev_dst, "frame is NULL\n");
      break;
    }

    memmove(frame->data, src_tx_ring->skbs[src_tx_ring->next_to_use]->data,
            bd->len);

    skb = src_tx_ring->skbs[src_tx_ring->next_to_use];
    dev_kfree_skb_any(skb);

    // print_ring();
    dst_rx_ring->next_to_use =
        (dst_rx_ring->next_to_use + 1) % dst_rx_ring->size;
    src_tx_ring->next_to_use =
        (src_tx_ring->next_to_use + 1) % src_tx_ring->size;
  }

  if (napi_schedule_prep(&dst->napi)) {
    __napi_schedule(&dst->napi);
  }

  kfree(work_ctx);
}
#endif

static netdev_tx_t nic_xmit_frame(struct sk_buff *skb,
                                  struct net_device *netdev) {
  struct nic_adapter *adapter = netdev_priv(netdev);
  struct nic_tx_ring *tx_ring;
  struct nic_bd *bd;
  u16 next_to_use;
#ifndef NO_PCI
  struct pci_dev *pdev = adapter->pdev;
#endif

  netdev_info(netdev, "nic_xmit_frame\n");
  netdev_info(netdev, "skb->len: %u\n", skb->len);

  // return NETDEV_TX_OK;

  /* This goes back to the question of how to logically map a Tx queue
   * to a flow.  Right now, performance is impacted slightly negatively
   * if using multiple Tx queues.  If the stack breaks away from a
   * single qdisc implementation, we can look at this again.
   */
  tx_ring = &adapter->tx_ring;
  next_to_use = tx_ring->next_to_use;
  bd = tx_ring->bd_va + next_to_use;

  /* On PCI/PCI-X HW, if packet size is less than ETH_ZLEN,
   * packets may get corrupted during padding by HW.
   * To WA this issue, pad all small packets manually.
   */
  if (eth_skb_pad(skb)) {
    netdev_err(netdev, "eth_skb_pad failed\n");
    return NETDEV_TX_OK;
  }

  // dev_kfree_skb_any(skb);
  // return NETDEV_TX_OK;

  // mss
  // TODO
  bd->len = cpu_to_le16(skb->len);
  tx_ring->skbs[next_to_use] = skb;
#ifndef NO_PCI
  bd->addr = cpu_to_le64(dma_map_single(
      &pdev->dev, tx_ring->skbs[next_to_use]->data, skb->len, DMA_TO_DEVICE));
#else
  // use va as pa, for test
  bd->addr = (dma_addr_t)NULL;
#endif

  tx_ring->next_to_use = (next_to_use + 1) % tx_ring->bd_size;
#ifndef NO_PCI
  /* Force memory writes to complete before letting h/w
   * know there are new descriptors to fetch.  (Only
   * applicable for weak-ordered memory model archs,
   * such as IA-64).
   */
  // dma_wmb();

  // TODO, delay setting tail
  writel(tx_ring->next_to_use,
         ((void *)adapter->io_addr) + NIC_DMA_CTL_TX_BD_TAIL);
#else
  // for test
  {
    struct test_work_ctx *work_ctx;
    work_ctx = kmalloc(sizeof(struct test_work_ctx), GFP_ATOMIC);
    if (!work_ctx) {
      netdev_err(netdev, "work_ctx kmalloc failed\n");
      return NETDEV_TX_OK;
    }
    work_ctx->src = adapter->if_id;
    if (adapter->if_id == 0) {
      work_ctx->dst = 1;
    } else {
      work_ctx->dst = 0;
    }

    // netdev_info(netdev, "INIT_WORK test_copy_data\n");
    INIT_WORK(&work_ctx->work, test_copy_data);
    if (!queue_work(nic_wq, &work_ctx->work)) {
      netdev_err(netdev, "schedule_work failed\n");
      kfree(work_ctx);
    }
  }
#endif

  return NETDEV_TX_OK;
}

static struct sk_buff *nic_receive_skb(struct nic_adapter *adapter) {
  struct net_device *netdev = adapter->netdev;
  struct sk_buff *skb = NULL;
  struct nic_rx_frame *frame;
  struct nic_bd *bd;
  u16 len;
  netdev_info(netdev, "nic_receive_skb\n");

  frame = adapter->rx_ring.data_vas[adapter->rx_ring.next_to_use];
  bd = &adapter->rx_ring.bd_va[adapter->rx_ring.next_to_use];
  len = le16_to_cpu(bd->len);
  if (len == 0) {
    netdev_info(netdev, "nic_receive_skb: data_len == 0\n");
    goto err_recv;
  }

  skb = napi_alloc_skb(&adapter->napi, len);
  if (!skb) {
    netdev_err(netdev, "napi_alloc_skb failed\n");
    goto err_recv;
  }
  skb_put_data(skb, frame->data, len);
  netdev_info(netdev, "nic_receive_skb->len: %u\n", skb->len);

err_recv:

  bd->flags &= ~NIC_BD_FLAG_VALID;
  adapter->rx_ring.next_to_use =
      (adapter->rx_ring.next_to_use + 1) % adapter->rx_ring.bd_size;

  // bd->flags |= NIC_BD_FLAG_USED;
  return skb;
}

static void nic_set_rx_mode(struct net_device *netdev) {
  netdev_info(netdev, "nic_set_rx_mode\n");
}

static int nic_set_mac(struct net_device *netdev, void *p) {
  netdev_info(netdev, "nic_set_mac\n");
  return 0;
}

static void nic_tx_timeout(struct net_device *dev, unsigned int txqueue) {
  netdev_info(dev, "nic_tx_timeout\n");
}

static int nic_change_mtu(struct net_device *netdev, int new_mtu) {
  netdev_info(netdev, "nic_change_mtu\n");
  return 0;
}

static int nic_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd) {
  netdev_info(netdev, "nic_ioctl\n");
  return 0;
}

// static int nic_vlan_rx_add_vid(struct net_device *netdev, __be16 proto,
//                                u16 vid) {
//   return 0;
// }

// static int nic_vlan_rx_kill_vid(struct net_device *netdev, __be16 proto,
//                                 u16 vid) {
//   return 0;
// }

static void nic_netpoll(struct net_device *netdev) {
  netdev_info(netdev, "nic_netpoll\n");
}

static int nic_poll(struct napi_struct *napi, int budget) {
  struct nic_adapter *adapter = container_of(napi, struct nic_adapter, napi);
  struct nic_rx_ring *rx_ring;
  struct sk_buff *skb;
  int work_done = 0;
  netdev_info(adapter->netdev, "nic_poll\n");

  // TODO
  // int work_to_do = min(*budget, netdev->quota);

  rx_ring = &adapter->rx_ring;

  netdev_info(adapter->netdev,
              "rx_ring->bd_va[rx_ring->next_to_use].flags = %u\n",
              rx_ring->bd_va[rx_ring->next_to_use].flags);

  while (rx_ring->bd_va[rx_ring->next_to_use].flags & NIC_BD_FLAG_VALID) {
    netdev_info(adapter->netdev, "nic_poll: next_to_use = %u\n",
                rx_ring->next_to_use);
    if (work_done >= budget) {
      break;
    }
    work_done++;

    skb = nic_receive_skb(adapter);
    if (!skb) {
      break;
    }
    skb->protocol = eth_type_trans(skb, adapter->netdev);
    napi_gro_receive(napi, skb);
  }

#ifndef NO_PCI
  if (napi_complete_done(napi, work_done)) {
    nic_set_int(adapter, NIC_VEC_RX, true);
  }
#endif

  return work_done;
}

static netdev_features_t nic_fix_features(struct net_device *netdev,
                                          netdev_features_t features) {
  /* Since there is no support for separate Rx/Tx vlan accel
   * enable/disable make sure Tx flag is always in same state as Rx.
   */
  if (features & NETIF_F_HW_VLAN_CTAG_RX) {
    features |= NETIF_F_HW_VLAN_CTAG_TX;
  } else {
    features &= ~NETIF_F_HW_VLAN_CTAG_TX;
  }

  return features;
}

static int nic_set_features(struct net_device *netdev,
                            netdev_features_t features) {
  // struct nic_adapter *adapter = netdev_priv(netdev);
  // netdev_features_t changed = features ^ netdev->features;

  // do nothing

  return 0;
}

#ifndef NO_PCI
static void nic_set_int(struct nic_adapter *adapter, int nr, bool enable) {
  void *csr_int_addr = adapter->io_addr + NIC_CSR_CTL_INT_OFFSET(nr);
  if (enable) {
    writel(0x01, csr_int_addr);
    netdev_info(adapter->netdev, "enable interrupt %d\n", nr);
  } else {
    writel(0x0, csr_int_addr);
    netdev_info(adapter->netdev, "disable interrupt %d\n", nr);
  }
}

static irqreturn_t nic_interrupt_tx(int irq, void *data) {
  struct net_device *netdev = data;
  struct nic_adapter *adapter = netdev_priv(netdev);
  struct clean_work_ctx *work_ctx;
  netdev_info(netdev, "nic_interrupt_tx\n");
  work_ctx = kmalloc(sizeof(struct clean_work_ctx), GFP_ATOMIC);
  if (!work_ctx) {
    netdev_err(netdev, "work_ctx kmalloc failed\n");
    return IRQ_HANDLED;
  }
  work_ctx->adapter = adapter;

  // netdev_info(netdev, "INIT_WORK nic_clean_tx_ring_work\n");
  INIT_WORK(&work_ctx->work, nic_clean_tx_ring_work);
  if (!queue_work(nic_wq, &work_ctx->work)) {
    netdev_err(netdev, "schedule_work failed\n");
    kfree(work_ctx);
  }
  return IRQ_HANDLED;
}
static irqreturn_t nic_interrupt_rx(int irq, void *data) {
  struct net_device *netdev = data;
  struct nic_adapter *adapter = netdev_priv(netdev);
  netdev_info(netdev, "nic_interrupt_rx\n");
  if (napi_schedule_prep(&adapter->napi)) {
    __napi_schedule(&adapter->napi);
  }
  nic_set_int(adapter, NIC_VEC_RX, false);
  return IRQ_HANDLED;
}

static void nic_clean_tx_ring_work(struct work_struct *work) {
  struct nic_adapter *adapter =
      container_of(work, struct clean_work_ctx, work)->adapter;
  struct nic_bd *bd_clean;
  struct sk_buff *skb_clean;
  struct nic_tx_ring *tx_ring = &adapter->tx_ring;
  netdev_info(adapter->netdev, "nic_clean_tx_ring_work\n");
  nic_set_int(adapter, NIC_VEC_TX, false);
  while (1) {
    if (!tx_ring->bd_va || !tx_ring->skbs) {
      break;
    }
    bd_clean = &tx_ring->bd_va[tx_ring->next_to_clean];
    skb_clean = tx_ring->skbs[tx_ring->next_to_clean];
    if (!(bd_clean->flags & NIC_BD_FLAG_USED)) {
      break;
    }
    dma_unmap_single(&adapter->pdev->dev, bd_clean->addr, bd_clean->len,
                     DMA_TO_DEVICE);
    dev_kfree_skb_any(skb_clean);
    // bd_clean->flags &= ~NIC_BD_FLAG_VALID;
    // bd_clean->flags &= ~NIC_BD_FLAG_USED;
    bd_clean->flags = 0;

    netdev_info(adapter->netdev, "free skb %u\n", tx_ring->next_to_clean);

    tx_ring->next_to_clean = (tx_ring->next_to_clean + 1) % tx_ring->bd_size;
  }
  nic_set_int(adapter, NIC_VEC_TX, true);
}
#endif // PCI_FN_TEST

#endif
