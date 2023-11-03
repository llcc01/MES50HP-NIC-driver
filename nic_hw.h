#ifndef _NIC_HW_H_
#define _NIC_HW_H_

#include "nic.h"

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


void nic_set_hw(struct nic_adapter *adapter);

#endif