#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>

#define BUFFER_SIZE 2048

// Create a TUN/TAP device
int create_tun_device(char *dev_name, int flags) {
    struct ifreq ifr;
    int tun_fd, err;

    if ((tun_fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("open");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = flags;

    if (dev_name != NULL) {
        strncpy(ifr.ifr_name, dev_name, IFNAMSIZ);
    }

    if ((err = ioctl(tun_fd, TUNSETIFF, (void *)&ifr)) < 0) {
        perror("ioctl");
        close(tun_fd);
        return err;
    }

    strcpy(dev_name, ifr.ifr_name);
    return tun_fd;
}

// Read packets from the TUN/TAP device
void read_packets(int tun_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t nread;

    while (1) {
        nread = read(tun_fd, buffer, sizeof(buffer));
        if (nread < 0) {
            perror("read");
            break;
        }

        // Process the received packet here
        printf("Received packet: %.*s\n", (int)nread, buffer);
    }
}

// Write packets to the TUN/TAP device
void write_packets(int tun_fd, const char *packet, size_t packet_len) {
    ssize_t nwritten = write(tun_fd, packet, packet_len);
    if (nwritten < 0) {
        perror("write");
    } else {
        printf("Sent packet: %.*s\n", (int)nwritten, packet);
    }
}

// Entry point of the program
int main() {
    int tun_fd;
    char tun_name[IFNAMSIZ];

    if (geteuid() != 0) {
        fprintf(stderr, "You need root privileges to run this program.\n");
        return 1;
    }

    tun_fd = create_tun_device(tun_name, IFF_TAP);  // Use IFF_TUN for TUN device

    if (tun_fd < 0) {
        fprintf(stderr, "Failed to create TUN/TAP device.\n");
        return 1;
    }

    printf("TUN/TAP device %s created.\n", tun_name);

    write_packets(tun_fd, "Hello, TUN/TAP!", 15);

    read_packets(tun_fd);

    close(tun_fd);
    printf("\nProgram terminated.\n");

    return 0;
}
