#ifndef _NIC_CDEV_H_
#define _NIC_CDEV_H_

#include "nic.h"
#include <linux/cdev.h>

#define NIC_IOC_MAGIC 'S'
#define NIC_IOC_NR_SET_HW 1

int nic_init_cdev(struct nic_drvdata *drvdata);

void nic_exit_cdev(struct nic_drvdata *drvdata);

#endif