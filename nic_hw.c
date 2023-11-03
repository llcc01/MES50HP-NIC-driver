#include "nic_hw.h"

void nic_set_hw(struct nic_adapter *adapter) {
  struct nic_tx_ring *tx_ring = &adapter->tx_ring;
  struct nic_rx_ring *rx_ring = &adapter->rx_ring;
  netdev_info(adapter->netdev, "tx_ring->bd_pa: %llx\n", tx_ring->bd_pa);
  writel(tx_ring->bd_pa & 0xffffffff,
         adapter->io_addr + NIC_DMA_CTL_TX_BD_BA_LOW);
  writel(tx_ring->bd_pa >> 32, adapter->io_addr + NIC_DMA_CTL_TX_BD_BA_HIGH);
  netdev_info(adapter->netdev, "rx_ring->bd_pa: %llx\n", rx_ring->bd_pa);
  writel(rx_ring->bd_pa & 0xffffffff,
         adapter->io_addr + NIC_DMA_CTL_RX_BD_BA_LOW);
  writel(rx_ring->bd_pa >> 32, adapter->io_addr + NIC_DMA_CTL_RX_BD_BA_HIGH);
}