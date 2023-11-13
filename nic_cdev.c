#include "nic_cdev.h"
#include "common.h"
#include "nic.h"
#include "nic_hw.h"
#include <linux/semaphore.h>

static struct semaphore raw_sem;

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

  sema_init(&raw_sem, 1);

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
  struct nic_cdev_data *cdev_data;
  PRINT_INFO("nic_cdev_open\n");
  cdev_data = kzalloc(sizeof(struct nic_cdev_data), GFP_KERNEL);
  cdev_data->cdev = inode->i_cdev;

  filp->private_data = cdev_data;
  return 0;
}

int nic_cdev_release(struct inode *inode, struct file *filp) {
  struct nic_cdev_data *cdev_data = filp->private_data;
  PRINT_INFO("nic_cdev_release\n");
  if (cdev_data->last_cmd == NIC_IOC_NR_RW_RAW) {
    up(&raw_sem);
  }
  kfree(cdev_data);
  return 0;
}

ssize_t nic_cdev_read(struct file *filp, char __user *buf, size_t count,
                      loff_t *f_pos) {
  struct nic_cdev_data *cdev_data = filp->private_data;
  struct nic_drvdata *drvdata =
      container_of(cdev_data->cdev, struct nic_drvdata, c_dev);
  struct nic_adapter *adapter = netdev_priv(drvdata->netdevs[cdev_data->if_id]);
  int err;
  // PRINT_INFO("nic_cdev_read\n");

  switch (_IOC_NR(cdev_data->last_cmd)) {
  case NIC_IOC_NR_RX_BD:
    if (*f_pos + count >= sizeof(struct nic_bd) * NIC_RX_RING_QUEUES) {
      PRINT_ERR("uio %d: read rx bd failed, overflow\n", cdev_data->if_id);
      return 0;
    }
    err = copy_to_user(buf, adapter->rx_ring.bd_va, count);
    if (err) {
      PRINT_ERR("uio %d: copy_to_user failed\n", cdev_data->if_id);
      return -EFAULT;
    }
    return count;
    break;
  case NIC_IOC_NR_RW_RAW:
    break;
  default:
    PRINT_ERR("invalid read cmd\n");
    break;
  }

  return 0;
}

ssize_t nic_cdev_write(struct file *filp, const char __user *buf, size_t count,
                       loff_t *f_pos) {
  PRINT_INFO("nic_cdev_write\n");
  return count;
}

long nic_cdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
  struct nic_cdev_data *cdev_data = filp->private_data;
  struct cdev *cdev = cdev_data->cdev;
  struct nic_drvdata *drvdata = container_of(cdev, struct nic_drvdata, c_dev);
  int i;

  // PRINT_INFO("nic_cdev_ioctl\n");
  if (_IOC_TYPE(cmd) != NIC_IOC_MAGIC) {
    return -ENOTTY;
  }

  cdev_data->last_cmd = cmd;

  switch (_IOC_NR(cmd)) {
  case NIC_IOC_NR_SET_HW:
    // PRINT_INFO("NIC_IOC_NR_SET_HW\n");
    for (i = 0; i < NIC_IF_NUM; i++) {
      struct nic_adapter *adapter = netdev_priv(drvdata->netdevs[i]);
      nic_set_hw(adapter);
    }
    break;
  case NIC_IOC_NR_RX_BD:
    // PRINT_INFO("NIC_IOC_NR_RX_BD\n");
    cdev_data->if_id = arg;
    break;
  case NIC_IOC_NR_UIO_EN:
    drvdata->uio_enabled = !!arg;
    break;
  case NIC_IOC_NR_RW_RAW:
    // PRINT_INFO("NIC_IOC_NR_RW_RAW\n");
    if (down_interruptible(&raw_sem)) {
      PRINT_ERR("busy\n");
      cdev_data->last_cmd = 0;
      return -EBUSY;
    }
    break;
  default:
    break;
  }
  return 0;
}

loff_t nic_cdev_llseek(struct file *filp, loff_t off, int whence) {
  PRINT_INFO("nic_cdev_llseek\n");
  return 0;
}