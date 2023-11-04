#include "app.h"
#include "common.h"

#include <asm-generic/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

int fd;

void poll_bd0() {
  struct nic_bd bd;
  struct tm *tm_t;
  struct timeval time;

  while (1) {
    APP_IOC_INT(fd, NIC_IOC_NR_RX_BD, 0);
    read(fd, &bd, sizeof(struct nic_bd));

    gettimeofday(&time, NULL);
    tm_t = localtime(&time.tv_sec);
    printf("%02d:%02d:%02d.%03ld if0 rx bd0: addr = %lx, len = %d, flags = %lx\n",
           tm_t->tm_hour, tm_t->tm_min, tm_t->tm_sec, time.tv_usec / 1000,
           bd.addr, bd.len, bd.flags);

    APP_IOC_INT(fd, NIC_IOC_NR_RX_BD, 1);
    read(fd, &bd, sizeof(struct nic_bd));

    gettimeofday(&time, NULL);
    tm_t = localtime(&time.tv_sec);
    printf("%02d:%02d:%02d.%03ld if1 rx bd0: addr = %lx, len = %d, flags = %lx\n",
           tm_t->tm_hour, tm_t->tm_min, tm_t->tm_sec, time.tv_usec / 1000,
           bd.addr, bd.len, bd.flags);
    usleep(1000);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <cmd>\n", argv[0]);
    return -1;
  }

  fd = open("/dev/" NIC_DRIVER_NAME, O_RDWR | O_SYNC);
  if (fd < 0) {
    printf("open hw failed\n");
    return -1;
  }

  if (strcmp(argv[1], "set_hw") == 0) {
    printf("set_hw\n");
    APP_IOC(fd, NIC_IOC_NR_SET_HW);
  } else if (strcmp(argv[1], "poll") == 0) {
    printf("poll rx bd0\n");
    printf("press ctrl+c to exit\n");
    sleep(1);
    poll_bd0();
  } else {
    printf("invalid argument\n");
    return -1;
  }

  return 0;
}