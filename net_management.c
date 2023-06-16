#include <string.h>
#include <net/if.h>
#include <net/if.h>
#include <stdlib.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/if_tun.h>

int create_tap_device(char *dev_name, int flags) {
    struct ifreq ifr;
    int err;
    int tap_fd;

    if ((tap_fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("open");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));

    // ifr_flags field to choose whether to create a TUN (i.e. IP) or a TAP (i.e. Ethernet)
    ifr.ifr_flags = flags;


    // ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    if (dev_name != NULL) {
        strncpy(ifr.ifr_name, dev_name, IFNAMSIZ);
    }

    if ((err = ioctl(tap_fd, TUNSETIFF, (void *)&ifr)) < 0) {
        perror("ioctl");
        close(tap_fd);
        return err;
    }

    strcpy(dev_name, ifr.ifr_name);
    return tap_fd;
}
