#include "nic.h"
#include <linux/dma-mapping.h>
#include <linux/version.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lc");
MODULE_DESCRIPTION("NIC driver module.");
MODULE_VERSION("0.01");

char nic_driver_name[] = NIC_DRIVER_NAME;

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
static int nic_poll(struct napi_struct *napi, int budget);
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
// .ndo_vlan_rx_add_vid = nic_vlan_rx_add_vid,
// .ndo_vlan_rx_kill_vid = nic_vlan_rx_kill_vid,
#ifdef CONFIG_NET_POLL_CONTROLLER
    .ndo_poll_controller = nic_netpoll,
#endif
    .ndo_fix_features = nic_fix_features,
    .ndo_set_features = nic_set_features,
};

#ifdef NO_PCI

static struct net_device *test_netdev[4];
static struct workqueue_struct *test_wq;

#endif

static int __init nic_init_module(void) {
  int ret;
  PRINT_INFO("nic_init_module\n");

#ifndef NO_PCI
  ret = pci_register_driver(&nic_driver);
#else
  ret = nic_probe(NULL, NULL);
  test_wq = create_singlethread_workqueue("test_wq");
#endif

  return ret;
}

module_init(nic_init_module);

static void __exit nic_exit_module(void) {
  PRINT_INFO("nic_exit_module\n");
#ifndef NO_PCI
  pci_unregister_driver(&nic_driver);
#else
  nic_remove(NULL);
  destroy_workqueue(test_wq);
#endif
}

module_exit(nic_exit_module);

static int nic_probe(struct pci_dev *pdev, const struct pci_device_id *ent) {
  struct net_device **netdevs;
  struct nic_adapter *adapter[IF_NUM];
  int err = 0;
  size_t i;

  PRINT_INFO("nic_probe\n");

  netdevs = kmalloc(sizeof(struct net_device *) * IF_NUM, GFP_KERNEL);
  if (!netdevs) {
    PRINT_ERR("alloc netdevs failed\n");
    err = -ENOMEM;
    goto err_alloc_netdevs;
  }
  // net device
  for (i = 0; i < IF_NUM; i++) {
    netdevs[i] = alloc_etherdev(sizeof(struct nic_adapter));
    adapter[i] = netdev_priv(netdevs[i]);
    if (!netdevs[i]) {
      PRINT_ERR("alloc_etherdev %zu failed\n", i);
      err = -ENOMEM;
      goto err_alloc_etherdev;
    }

    adapter[i]->netdev = netdevs[i];
    adapter[i]->if_id = i;
#ifndef NO_PCI
    adapter[i]->pdev = pdev;
#else
    adapter[i]->pdev = NULL;
#endif
  }

#ifndef NO_PCI
  for (i = 0; i < IF_NUM; i++) {
    SET_NETDEV_DEV(netdevs[i], &pdev->dev);
  }
  pci_set_drvdata(pdev, netdevs);
#else
  memmove(test_netdev, netdevs, sizeof(void *) * IF_NUM);
#endif
  PRINT_INFO("alloc netdev\n");

  // io
  PRINT_INFO("pci_enable_device\n");

  // dma
  PRINT_INFO("pci_set_dma_mask\n");

  // ops
  for (i = 0; i < IF_NUM; i++) {
    // char mac_addr[ETH_ALEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    netdevs[i]->netdev_ops = &nic_netdev_ops;
    nic_set_ethtool_ops(netdevs[i]);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
    netif_napi_add(netdevs[i], &adapter[i]->napi, nic_poll);
#else
    netif_napi_add(netdevs[i], &adapter[i]->napi, nic_poll, NAPI_POLL_WEIGHT);
#endif
    // eth_hw_addr_set(netdev[i], adapter[i]->mac_addr);
    eth_hw_addr_random(netdevs[i]);
  }
  PRINT_INFO("set ops\n");

  for (i = 0; i < IF_NUM; i++) {
    err = register_netdev(netdevs[i]);
    if (err) {
      PRINT_ERR("register_netdev %zu failed\n", i);
      goto err_register;
    }
    netif_carrier_off(netdevs[i]);
  }
  PRINT_INFO("register netdev\n");

  PRINT_INFO("nic_probe done\n");
  return 0;

err_register:
err_alloc_etherdev:
  kfree(netdevs);
err_alloc_netdevs:
  return err;
}

static void nic_remove(struct pci_dev *pdev) {
  struct net_device **netdevs;
  size_t i;
#ifndef NO_PCI
  netdevs = pci_get_drvdata(pdev);
#else
  netdevs = test_netdev;
#endif
  PRINT_INFO("nic_remove\n");

  // net device
  for (i = 0; i < IF_NUM; i++) {
    unregister_netdev(netdevs[i]);
    free_netdev(netdevs[i]);
  }
  kfree(netdevs);
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
  dma_addr_t rx_buffer_pa;
  size_t i;

  // TX
  tx_ring->queues = NIC_TX_RING_QUEUES;
  // tx_ring->size = sizeof(struct nic_tx_desc) * tx_ring->queues;
  // tx_ring->size = ALIGN(tx_ring->size, 4096);

  tx_ring->data_vas = kcalloc(tx_ring->queues, sizeof(void *), GFP_KERNEL);
  if (!tx_ring->data_vas) {
    PRINT_ERR("alloc tx_ring data_vas failed\n");
    err = -ENOMEM;
    goto err_tx;
  }

  // TX BD

#ifndef NO_PCI
  tx_ring->bd_va =
      dma_alloc_coherent(&pdev->dev, sizeof(struct nic_bd) * tx_ring->queues,
                         &rx_ring->bd_pa, GFP_KERNEL);
#else
  tx_ring->bd_va = kcalloc(tx_ring->queues, sizeof(struct nic_bd), GFP_KERNEL);
#endif

  if (!tx_ring->bd_va) {
    PRINT_ERR("alloc tx_ring bd failed\n");
    err = -ENOMEM;
    goto err_tx_bd;
  }

  // RX

  rx_ring->queues = NIC_RX_RING_QUEUES;
  // rx_ring->size = sizeof(struct nic_rx_frame) * rx_ring->queues;
  // rx_ring->size = ALIGN(rx_ring->size, 4096);

  rx_ring->data_vas = kcalloc(rx_ring->queues, sizeof(void *), GFP_KERNEL);

  if (!rx_ring->data_vas) {
    PRINT_ERR("alloc rx_ring vas failed\n");
    err = -ENOMEM;
    goto err_rx;
  }

  // RX buffer
  // contiguous

#ifndef NO_PCI
  rx_ring->data_vas[0] = dma_alloc_coherent(
      &pdev->dev, sizeof(struct nic_rx_frame) * rx_ring->queues, &rx_buffer_pa,
      GFP_KERNEL);
#else
  rx_ring->data_vas[0] =
      kcalloc(rx_ring->queues, sizeof(struct nic_rx_frame), GFP_KERNEL);
#endif

  if (!rx_ring->data_vas[0]) {
    PRINT_ERR("alloc rx_ring buffer failed\n");
    err = -ENOMEM;
    goto err_rx_buffer;
  }

  for (i = 1; i < rx_ring->queues; i++) {
    rx_ring->data_vas[i] =
        rx_ring->data_vas[i - 1] + sizeof(struct nic_rx_frame);
  }

  // RX BD
#ifndef NO_PCI
  rx_ring->bd_va = dma_alloc_coherent(&pdev->dev, rx_ring->queues,
                                      &rx_ring->bd_pa, GFP_KERNEL);
  for (i = 0; i < rx_ring->queues; i++) {
    rx_ring->bd_va[i].addr = rx_buffer_pa + sizeof(struct nic_bd) * i;
  }
#else
  rx_bd_va = kcalloc(rx_ring->queues, sizeof(struct nic_bd), GFP_KERNEL);
#endif

  if (!rx_ring->data_vas) {
    PRINT_ERR("dma_alloc_coherent rx_ring bd failed\n");
    err = -ENOMEM;
    goto err_rx_bd;
  }

  // RX head
#ifndef NO_PCI
  rx_ring->head_va = dma_alloc_coherent(&pdev->dev, sizeof(*(rx_ring->head_va)),
                                        &adapter->rx_ring.head_pa, GFP_KERNEL);
#else
  rx_ring->head_va = kcalloc(1, sizeof(*(rx_ring->head_va)), GFP_KERNEL);
  rx_ring->head_pa = (dma_addr_t)NULL;
#endif

  if (!rx_ring->head_va) {
    PRINT_ERR("dma_alloc_coherent rx_ring head failed\n");
    err = -ENOMEM;
    goto err_rx_head_tail;
  }

  // check_64k_bound
  // TODO

  return 0;

err_rx_head_tail:
#ifndef NO_PCI
  dma_free_coherent(&pdev->dev, rx_ring->queues, rx_ring->data_vas,
                    rx_ring->bd_va->addr);
#else
  kfree(rx_ring->bd_va);
#endif

err_rx_bd:
#ifndef NO_PCI
  dma_free_coherent(&pdev->dev, rx_ring->queues, rx_ring->data_vas[0],
                    rx_ring->bd_pa);
#else
  kfree(rx_ring->data_vas[0]);
#endif

err_rx_buffer:
  kfree(rx_ring->data_vas);

err_rx:
#ifndef NO_PCI
  dma_free_coherent(&pdev->dev, tx_ring->queues, tx_ring->bd_va,
                    tx_ring->bd_pa);
#else
  kfree(tx_ring->bd_va);

#endif

err_tx_bd:
  kfree(tx_ring->data_vas);

err_tx:
  return err;
}

static int nic_free_queues(struct nic_adapter *adapter) {
  struct nic_tx_ring *tx_ring = &adapter->tx_ring;
  struct nic_rx_ring *rx_ring = &adapter->rx_ring;

#ifndef NO_PCI
  struct pci_dev *pdev = adapter->pdev;
  dma_free_coherent(&pdev->dev, sizeof(*rx_ring->head_va), rx_ring->head_va,
                    rx_ring->head_pa);
  dma_free_coherent(&pdev->dev, rx_ring->queues, rx_ring->data_vas[0],
                    rx_ring->bd_va[0].addr);
  dma_free_coherent(&pdev->dev, rx_ring->queues, rx_ring->bd_va,
                    rx_ring->bd_pa);
  kfree(rx_ring->data_vas);

  dma_free_coherent(&pdev->dev, tx_ring->queues, tx_ring->bd_va,
                    tx_ring->bd_pa);
  kfree(tx_ring->data_vas);
#else
  kfree(rx_ring->head_va);
  kfree(rx_ring->data_vas[0]);
  kfree(rx_ring->bd_va);
  kfree(rx_ring->data_vas);

  kfree(tx_ring->bd_va);
  kfree(tx_ring->data_vas);
#endif

  tx_ring->queues = 0;
  // tx_ring->size = 0;
  rx_ring->queues = 0;
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

  nic_free_queues(adapter);
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

  nic_free_all_resources(adapter);

  return 0;
}

// for test
#ifdef NO_PCI
void print_ring(void) {
  size_t i;
  for (i = 0; i < IF_NUM; i++) {
    struct nic_adapter *adapter = netdev_priv(test_netdev[i]);
    struct nic_tx_ring *tx_ring = &adapter->tx_ring;
    struct nic_rx_ring *rx_ring = &adapter->rx_ring;

    PRINT_INFO("if_id: %u\n", adapter->if_id);
    PRINT_INFO("tx_ring->queues: %u\n", tx_ring->queues);
    PRINT_INFO("tx_ring->head: %u\n", tx_ring->head);
    PRINT_INFO("tx_ring->tail: %u\n", tx_ring->tail);
    PRINT_INFO("rx_ring->queues: %u\n", rx_ring->queues);
    PRINT_INFO("rx_ring->head: %u\n", *(rx_ring->head_va));
    PRINT_INFO("rx_ring->tail: %u\n", rx_ring->tail);
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

  PRINT_INFO("test_copy_data called\n");

  while (src_tx_ring->tail != src_tx_ring->head) {
    struct nic_bd *bd = &src_tx_ring->bd_va[src_tx_ring->tail];
    struct nic_rx_frame *frame = dst_rx_ring->data_vas[*(dst_rx_ring->head_va)];

    frame->data_len = bd->len;

    memmove(frame->data, src_tx_ring->data_vas[src_tx_ring->tail], bd->len);

    PRINT_INFO("test_copy_data %u->%u: %u\n", src->if_id, dst->if_id, bd->len);
    // print_ring();
    *(dst_rx_ring->head_va) =
        (*(dst_rx_ring->head_va) + 1) % dst_rx_ring->queues;
    src_tx_ring->tail = (src_tx_ring->tail + 1) % src_tx_ring->queues;
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
  u16 head;
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
  head = tx_ring->head;
  bd = tx_ring->bd_va + head;

  /* On PCI/PCI-X HW, if packet size is less than ETH_ZLEN,
   * packets may get corrupted during padding by HW.
   * To WA this issue, pad all small packets manually.
   */
  if (eth_skb_pad(skb)) {
    netdev_err(netdev, "eth_skb_pad failed\n");
    return NETDEV_TX_OK;
  }

  // mss
  // TODO

  bd->len = skb->len;
  tx_ring->data_vas[head] = skb->data;
#ifndef NO_PCI
  bd->addr =
      dma_map_single(&pdev->dev, tx_ring->data_vas[head], bd->len, DMA_TO_DEVICE);
#else
  // use va as pa, for test
  bd->addr = (dma_addr_t)NULL;
#endif

  tx_ring->head = (head + 1) % tx_ring->queues;
#ifndef NO_PCI
  writel(tx_ring->head, ((void *)adapter->hw_addr) + NIC_MMIO_TX_BD_HEAD);
#endif

// for test
#ifdef NO_PCI
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
    if (!queue_work(test_wq, &work_ctx->work)) {
      netdev_err(netdev, "schedule_work failed\n");
      kfree(work_ctx);
    }
  }
#endif

  return NETDEV_TX_OK;
}

static struct sk_buff *nic_receive_skb(struct nic_adapter *adapter) {
  struct net_device *netdev = adapter->netdev;
  struct sk_buff *skb;
  struct nic_rx_frame *frame;
  netdev_info(netdev, "nic_receive_skb\n");

  frame = adapter->rx_ring.data_vas[adapter->rx_ring.tail];
  if (frame->data_len == 0) {
    return NULL;
  }
  skb = napi_alloc_skb(&adapter->napi, frame->data_len);
  if (!skb) {
    netdev_err(netdev, "napi_alloc_skb failed\n");
    return NULL;
  }
  skb_put_data(skb, frame->data, frame->data_len);
  netdev_info(netdev, "nic_receive_skb->len: %u\n", skb->len);
  adapter->rx_ring.tail = (adapter->rx_ring.tail + 1) % adapter->rx_ring.queues;

  frame->data_len = 0;
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

  while (*(rx_ring->head_va) != rx_ring->tail) {

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

  napi_complete_done(napi, work_done);

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
