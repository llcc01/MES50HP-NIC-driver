#ifndef _COMMON_H_
#define _COMMON_H_

#ifdef __KERNEL__

#include <linux/dma-mapping.h>
#include <linux/types.h>
#else
#include <stdint.h>

typedef uint64_t dma_addr_t;

#endif

#define NIC_DRIVER_NAME "pangonic"

#define NIC_IF_NUM 2

#define NIC_RX_PKT_SIZE 2048

#define NIC_TX_RING_QUEUES 8

#define NIC_RX_RING_QUEUES 8

#define NIC_IOC_MAGIC 'S'

#define NIC_IOC_NR_SET_HW 1

#define NIC_IOC_NR_RX_BD 2

struct nic_bd {
  union {
    uint64_t flags;
    uint16_t len;
  };
  dma_addr_t addr;
};

struct nic_rx_frame {
  uint8_t data[NIC_RX_PKT_SIZE];
};

#endif