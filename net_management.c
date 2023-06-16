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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net_management.h>

int create_tap_device(char *dev_name) {
    int err = 0;
    int tap_fd = 0;

    if ((tap_fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("Error opening /dev/net/tun: ");
        return tap_fd;
    }

    // ifr_flags field to choose whether to create a TUN (i.e. IP) or a TAP (i.e. Ethernet)
    struct ifreq ifr = {0};
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    if (dev_name != NULL) {
        strncpy(ifr.ifr_name, dev_name, IFNAMSIZ);
    }

    if ((err = ioctl(tap_fd, TUNSETIFF, (void *)&ifr)) < 0) {
        perror("ioctl TUNSETIFF");
        close(tap_fd);
        return err;
    }

    // TODO: Unsafe
    strcpy(dev_name, ifr.ifr_name);
    return tap_fd;
}

int get_interface_details(const char *interface_name, char *ip_address, char *netmask_address) {

    int err = 0;
    // Create a socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return sockfd;
    }

    // Set the interface name
    struct ifreq ifr;
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);

    // Get the IP address
    // ioctl: input and output control, talk to device drivers
    if ((err = ioctl(sockfd, SIOCGIFADDR, &ifr)) < 0) {
        perror("ioctl (SIOCGIFADDR)");
        close(sockfd);
        return err;
    }

    struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
    // sin_addr: the IP address in the socket
    if( ip_address ) {
        // Convert IP addr to string
        inet_ntop(AF_INET, &(addr->sin_addr), ip_address, INET_ADDRSTRLEN); // get
    }

    // Get the netmask
    if ((err = ioctl(sockfd, SIOCGIFNETMASK, &ifr)) < 0) {
        perror("ioctl (SIOCGIFNETMASK)");
        close(sockfd);
        return err;
    }
    addr = (struct sockaddr_in *)&ifr.ifr_netmask; // ifr.ifr_netmask ???
    if( netmask_address ) {
        inet_ntop(AF_INET, &(addr->sin_addr), netmask_address, INET_ADDRSTRLEN);
    }

    // Close the socket
    close(sockfd);
    return 0;
}

// function 2: grasp MAC address (physical address)

int get_mac_address(const char *interface_name, char *mac_address) {
    int err = 0;

    // Create a socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return sockfd;
    }

    // Set the interface name
    struct ifreq ifr;
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);

    // Get the MAC address
    if ((err = ioctl(sockfd, SIOCGIFHWADDR, &ifr)) < 0) {
        perror("ioctl (SIOCGIFHWADDR)");
        close(sockfd);
        return err;
    }

    unsigned char *mac = (unsigned char *)ifr.ifr_hwaddr.sa_data; // ifr.ifr_hwaddr ???
    // the MAC address is formatted as a string in the format XX:XX:XX:XX:XX:XX and stored in the mac_address buffer.
    sprintf(mac_address, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);


    // Close the socket
    close(sockfd);
    return 0;

}


// function 3: modify the IP and mask of "tun0"
// input: tun0, ip, mask; output: null

int modify_interface_details(const char *interface_name, const char *new_ip_address, const char *new_netmask) {

    int err = 0;
    // Create a socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return sockfd;
    }

    // Set the interface name
    struct ifreq ifr;
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);

    // Get the current flags of the interface
    if ((err = ioctl(sockfd, SIOCGIFFLAGS, &ifr)) < 0) {
        perror("ioctl (SIOCGIFFLAGS)");
        close(sockfd);
        return err;
    }

    // Disable the interface before modifying its address
    ifr.ifr_flags &= ~IFF_UP;
    if ((err = ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)) {
        perror("ioctl (SIOCSIFFLAGS)");
        close(sockfd);
        return err;
    }

    // Set the new IP address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    // int inet_pton(int af, const char * src, void * dst);
    inet_pton(AF_INET, new_ip_address, &(addr.sin_addr));

    // void *memcpy(void *dest, const void * src, size_t n)
    memcpy(&ifr.ifr_addr, &addr, sizeof(struct sockaddr));

    if (( err = ioctl(sockfd, SIOCSIFADDR, &ifr)) < 0) {
        perror("ioctl (SIOCSIFADDR)");
        close(sockfd);
        return err;
    }

    // Set the new netmask
    struct sockaddr_in netmask;
    memset(&netmask, 0, sizeof(netmask));
    netmask.sin_family = AF_INET;
    inet_pton(AF_INET, new_netmask, &(netmask.sin_addr));
    memcpy(&ifr.ifr_netmask, &netmask, sizeof(struct sockaddr)); // ifr.ifr_netmask ???

    if ((err = ioctl(sockfd, SIOCSIFNETMASK, &ifr)) < 0) {
        perror("ioctl (SIOCSIFNETMASK)");
        close(sockfd);
        return err;
    }

    // Enable the interface
    ifr.ifr_flags |= IFF_UP;
    if ((err = ioctl(sockfd, SIOCSIFFLAGS, &ifr)) < 0) {
        perror("ioctl (SIOCSIFFLAGS)");
        close(sockfd);
        return err;
    }

    // Close the socket
    close(sockfd);
    return 0;
}


// function 4: delete IP address and netmask of "enp0s1" interface
int delete_interface_details(const char *interface_name) {
    int err = 0;
    // Create a socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return sockfd;
    }

    // Set the interface name
    struct ifreq ifr;
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);

    // Delete the IP address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "", &(addr.sin_addr));
    memcpy(&ifr.ifr_addr, &addr, sizeof(struct sockaddr));

    if (( err = ioctl(sockfd, SIOCSIFADDR, &ifr)) < 0) {
        perror("ioctl (SIOCSIFADDR)");
        close(sockfd);
        return err;
    }

    // Close the socket
    close(sockfd);
    return 0;
}

/*
    replace tap0's MAC address with enp0s1's to keep consistency
*/


int modify_mac_address(const char *interface_name, const char *new_mac_address) {
    int sockfd;
    struct ifreq ifr;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    // Set the interface name
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);

    // Get the current MAC address
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl (SIOCGIFHWADDR)");
        close(sockfd);
        return -1;
    }

    // Set the new MAC address
    sscanf(new_mac_address, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &ifr.ifr_hwaddr.sa_data[0], &ifr.ifr_hwaddr.sa_data[1],
           &ifr.ifr_hwaddr.sa_data[2], &ifr.ifr_hwaddr.sa_data[3],
           &ifr.ifr_hwaddr.sa_data[4], &ifr.ifr_hwaddr.sa_data[5]);

    if (ioctl(sockfd, SIOCSIFHWADDR, &ifr) < 0) {
        perror("ioctl (SIOCSIFHWADDR)");
        close(sockfd);
        return -1;
    }

    // Close the socket
    close(sockfd);

    return 0;
}

