#ifndef _NIC_CDEV_H_
#define _NIC_CDEV_H_

#include "nic.h"
#include <linux/cdev.h>


int nic_init_cdev(struct nic_drvdata *drvdata);

void nic_exit_cdev(struct nic_drvdata *drvdata);

struct nic_cdev_data {
  struct cdev *cdev;
  int last_cmd;
  u16 if_id;
};

#endif