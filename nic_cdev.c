#include "nic_cdev.h"
#include "nic.h"
#include "nic_hw.h"

int nic_cdev_open(struct inode *inode, struct file *filp);
int nic_cdev_release(struct inode *inode, struct file *filp);
ssize_t nic_cdev_read(struct file *filp, char __user *buf, size_t count,
                      loff_t *f_pos);
ssize_t nic_cdev_write(struct file *filp, const char __user *buf, size_t count,
                       loff_t *f_pos);
long nic_cdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
loff_t nic_cdev_llseek(struct file *filp, loff_t off, int whence);

struct file_operations nic_fops = {
    .owner = THIS_MODULE,
    .open = nic_cdev_open,
    .release = nic_cdev_release,
    .read = nic_cdev_read,
    .write = nic_cdev_write,
    .unlocked_ioctl = nic_cdev_ioctl,
    .llseek = nic_cdev_llseek,
};

int nic_init_cdev(struct nic_drvdata *drvdata) {
  int err = 0;
  struct cdev *cdev = &drvdata->c_dev;
  PRINT_INFO("nic_init_cdev\n");

  err = alloc_chrdev_region(&drvdata->c_dev_no, 0, 1, NIC_DRIVER_NAME);
  if (err < 0) {
    PRINT_ERR("alloc_chrdev_region failed\n");
    goto err_alloc_chrdev_region;
  }

  cdev_init(cdev, &nic_fops);

  err = cdev_add(cdev, drvdata->c_dev_no, 1);
  if (err) {
    PRINT_ERR("cdev_add failed\n");
    goto err_cdev_add;
  }

  drvdata->c_dev_class = class_create(THIS_MODULE, NIC_DRIVER_NAME);
  if (IS_ERR(drvdata->c_dev_class)) {
    PRINT_ERR("class_create failed\n");
    err = PTR_ERR(drvdata->c_dev_class);
    goto err_class_create;
  }

  device_create(drvdata->c_dev_class, NULL, drvdata->c_dev_no, NULL,
                NIC_DRIVER_NAME);

  return 0;

err_class_create:

  cdev_del(cdev);
err_cdev_add:

  unregister_chrdev_region(drvdata->c_dev_no, 1);
err_alloc_chrdev_region:

  return err;
}

void nic_exit_cdev(struct nic_drvdata *drvdata) {
  PRINT_INFO("exit_cdev\n");
  device_destroy(drvdata->c_dev_class, drvdata->c_dev_no);
  class_destroy(drvdata->c_dev_class);
  cdev_del(&drvdata->c_dev);
  unregister_chrdev_region(drvdata->c_dev_no, 1);
}

int nic_cdev_open(struct inode *inode, struct file *filp) {
  PRINT_INFO("nic_cdev_open\n");
  filp->private_data = inode->i_cdev;
  return 0;
}

int nic_cdev_release(struct inode *inode, struct file *filp) {
  PRINT_INFO("nic_cdev_release\n");
  return 0;
}

ssize_t nic_cdev_read(struct file *filp, char __user *buf, size_t count,
                      loff_t *f_pos) {
  PRINT_INFO("nic_cdev_read\n");
  return 0;
}

ssize_t nic_cdev_write(struct file *filp, const char __user *buf, size_t count,
                       loff_t *f_pos) {
  PRINT_INFO("nic_cdev_write\n");
  return count;
}

long nic_cdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
  struct cdev *cdev = filp->private_data;
  struct nic_drvdata *drvdata = container_of(cdev, struct nic_drvdata, c_dev);
  int i;
  PRINT_INFO("nic_cdev_ioctl\n");
  if (_IOC_TYPE(cmd) != NIC_IOC_MAGIC) {
    return -ENOTTY;
  }

  switch (_IOC_NR(cmd)) {
  case NIC_IOC_NR_SET_HW:
    PRINT_INFO("NIC_IOC_NR_SET_HW\n");
    for (i = 0; i < NIC_IF_NUM; i++) {
      struct nic_adapter *adapter = netdev_priv(drvdata->netdevs[i]);
      nic_set_hw(adapter);
    }
    break;
  }
  return 0;
}

loff_t nic_cdev_llseek(struct file *filp, loff_t off, int whence) {
  PRINT_INFO("nic_cdev_llseek\n");
  return 0;
}