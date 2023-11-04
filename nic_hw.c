#include "nic_hw.h"

void nic_set_hw(struct nic_adapter *adapter) {
  struct nic_tx_ring *tx_ring = &adapter->tx_ring;
  struct nic_rx_ring *rx_ring = &adapter->rx_ring;

  netdev_info(adapter->netdev, "tx_ring->bd_pa: %llx\n", tx_ring->bd_pa);
  writel(tx_ring->bd_pa & 0xffffffff,
         adapter->io_addr + NIC_REG_TO_ADDR(NIC_PCIE_REG_TX_BD_BA_LOW));
  writel(tx_ring->bd_pa >> 32,
         adapter->io_addr + NIC_REG_TO_ADDR(NIC_PCIE_REG_TX_BD_BA_HIGH));

  netdev_info(adapter->netdev, "tx_ring->bd_size: %d\n", tx_ring->bd_size);
  writel(tx_ring->bd_size,
         adapter->io_addr + NIC_REG_TO_ADDR(NIC_PCIE_REG_TX_BD_SIZE));

  netdev_info(adapter->netdev, "rx_ring->bd_pa: %llx\n", rx_ring->bd_pa);
  writel(rx_ring->bd_pa & 0xffffffff,
         adapter->io_addr + NIC_REG_TO_ADDR(NIC_PCIE_REG_RX_BD_BA_LOW));
  writel(rx_ring->bd_pa >> 32,
         adapter->io_addr + NIC_REG_TO_ADDR(NIC_PCIE_REG_RX_BD_BA_HIGH));

  netdev_info(adapter->netdev, "rx_ring->bd_size: %d\n", rx_ring->bd_size);
  writel(rx_ring->bd_size,
         adapter->io_addr + NIC_REG_TO_ADDR(NIC_PCIE_REG_RX_BD_SIZE));
}

void nic_unset_hw(struct nic_adapter *adapter) {
  writel(0, adapter->io_addr + NIC_REG_TO_ADDR(NIC_PCIE_REG_TX_BD_BA_LOW));
  writel(0, adapter->io_addr + NIC_REG_TO_ADDR(NIC_PCIE_REG_TX_BD_BA_HIGH));
  writel(0, adapter->io_addr + NIC_REG_TO_ADDR(NIC_PCIE_REG_TX_BD_SIZE));

  writel(0, adapter->io_addr + NIC_REG_TO_ADDR(NIC_PCIE_REG_RX_BD_BA_LOW));
  writel(0, adapter->io_addr + NIC_REG_TO_ADDR(NIC_PCIE_REG_RX_BD_BA_HIGH));
  writel(0, adapter->io_addr + NIC_REG_TO_ADDR(NIC_PCIE_REG_RX_BD_SIZE));
}

void nic_set_int(struct nic_adapter *adapter, int nr, bool enable) {
  void *csr_int_addr =
      adapter->io_addr + NIC_REG_TO_ADDR(NIC_PCIE_REG_INT_OFFSET(nr));
  if (enable) {
    writel(0x01, csr_int_addr);
    netdev_info(adapter->netdev, "enable interrupt %d\n", nr);
  } else {
    writel(0x0, csr_int_addr);
    netdev_info(adapter->netdev, "disable interrupt %d\n", nr);
  }
}