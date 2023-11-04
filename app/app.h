#ifndef _APP_H_
#define _APP_H_

#include <linux/ioctl.h> 
#include "common.h"

#define APP_IOC(fd,nr) ioctl(fd, _IOWR(NIC_IOC_MAGIC, nr, int), NULL);
#define APP_IOC_INT(fd,nr,arg) ioctl(fd, _IOWR(NIC_IOC_MAGIC, nr, int), arg);

#endif