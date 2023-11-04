#ifndef _NIC_HW_H_
#define _NIC_HW_H_

#include "nic.h"

// mmio

#define NIC_CTL_ADDR(func, ch, reg)                                            \
  (((func) << 16) + (((ch) & (BIT(7) - 1)) << 9) +                             \
   (((reg) & (BIT(7) - 1)) << 2))

#define NIC_IF_REG_SIZE BIT(9)

#define NIC_FUNC_ID_PCIE 0xf

#define NIC_REG_TO_ADDR(reg) ((reg) << 2)

#define NIC_ADDR_TO_REG(addr) (((addr) >> 2) & (BIT(7) - 1))

#define NIC_PCIE_REG(s, nr) (((s) << 3) + (nr))

// tx

#define NIC_PCIE_REG_TX_BD_BA_LOW NIC_PCIE_REG(0x0, 0x0)

#define NIC_PCIE_REG_TX_BD_BA_HIGH NIC_PCIE_REG(0x0, 0x1)

#define NIC_PCIE_REG_TX_BD_SIZE NIC_PCIE_REG(0x0, 0x2)

// #define NIC_PCIE_REG_TX_BD_HEAD NIC_PCIE_REG(0x0, 0x3)

#define NIC_PCIE_REG_TX_BD_TAIL NIC_PCIE_REG(0x0, 0x4)

// rx

#define NIC_PCIE_REG_RX_BD_BA_LOW NIC_PCIE_REG(0x1, 0x0)

#define NIC_PCIE_REG_RX_BD_BA_HIGH NIC_PCIE_REG(0x1, 0x1)

#define NIC_PCIE_REG_RX_BD_SIZE NIC_PCIE_REG(0x1, 0x2)

// #define NIC_PCIE_REG_RX_BD_HEAD NIC_PCIE_REG(0x1, 0x3)

#define NIC_PCIE_REG_RX_BD_TAIL NIC_PCIE_REG(0x1, 0x4)

// interrupt

#define NIC_PCIE_REG_INT_OFFSET(tx_rx) NIC_PCIE_REG(0x2, (tx_rx))

// vector

#define NIC_VEC_TX 0

#define NIC_VEC_RX 1

#define NIC_VEC_IF_SIZE 2

// flags

#define NIC_BD_FLAG_VALID BIT(0)

#define NIC_BD_FLAG_USED BIT(1)

void nic_set_hw(struct nic_adapter *adapter);

void nic_unset_hw(struct nic_adapter *adapter);

void nic_set_int(struct nic_adapter *adapter, int nr, bool enable);

#endif