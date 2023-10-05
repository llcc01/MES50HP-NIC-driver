#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>

#define PCI_VENDOR_ID_MY 0x11cc

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lc");
MODULE_DESCRIPTION("NIC driver module.");
MODULE_VERSION("0.01");

char nic_driver_name[] = "nic";

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

static int __init nic_init_module(void) {
  int ret;
  printk(KERN_INFO "nic_init_module\n");
  // ret = pci_register_driver(&nic_driver);
  ret = 0;

  return ret;
}

module_init(nic_init_module);

static void __exit nic_exit_module(void) {
  printk(KERN_INFO "nic_exit_module\n");
  // pci_unregister_driver(&nic_driver);
}

module_exit(nic_exit_module);

static int nic_probe(struct pci_dev *pdev, const struct pci_device_id *ent) {
  printk(KERN_INFO "nic_probe\n");
  return 0;
}

static void nic_remove(struct pci_dev *pdev) {
  printk(KERN_INFO "nic_remove\n");
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
