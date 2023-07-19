#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_link.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/icmp.h>
#include <linux/bpf.h>
#include <linux/bpf_common.h>
#include <linux/if_packet.h>
#include <linux/filter.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/sockios.h>
#include <string.h>

#define MAX_PACKET_SIZE 4096
#define INTERFACE_NAME "eth0"  // Change this to match your interface

// eBPF program
static const struct sock_filter eBPF_prog[] = {
    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, 4),          // Load 32-bit length field
    BPF_STMT(BPF_RET | BPF_K, (u_int32_t)-1),       // Return -1
};

static const struct sock_fprog eBPF_prog_struct = {
    .len = sizeof(eBPF_prog) / sizeof(eBPF_prog[0]),
    .filter = (struct sock_filter *)eBPF_prog,
};

int main() {
    int prog_fd, sock_fd;
    struct sockaddr_ll sa;
    char packet[MAX_PACKET_SIZE];

    // Compile and load eBPF program
    prog_fd = socket(AF_PACKET, SOCK_RAW | SOCK_CLOEXEC, htons(ETH_P_ALL));
    if (prog_fd < 0) {
        perror("Failed to open socket");
        return 1;
    }

    if (setsockopt(prog_fd, SOL_SOCKET, SO_ATTACH_FILTER, &eBPF_prog_struct, sizeof(eBPF_prog_struct)) < 0) {
        perror("Failed to attach eBPF program");
        return 1;
    }

    // Open socket
    sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_fd < 0) {
        perror("Failed to open socket");
        return 1;
    }

    // Set interface index
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, INTERFACE_NAME, IFNAMSIZ);
    if (ioctl(sock_fd, SIOCGIFINDEX, &ifr) < 0) {
        perror("Failed to get interface index");
        return 1;
    }

    // Enable PACKET_TIMESTAMP
    int enable_timestamp = 1;
    if (setsockopt(sock_fd, SOL_PACKET, PACKET_TIMESTAMP, &enable_timestamp, sizeof(enable_timestamp)) < 0) {
        perror("Failed to enable PACKET_TIMESTAMP");
        return 1;
    }

    // Enable PACKET_FANOUT
    int enable_fanout = PACKET_FANOUT_HASH;
    if (setsockopt(sock_fd, SOL_PACKET, PACKET_FANOUT, &enable_fanout, sizeof(enable_fanout)) < 0) {
        perror("Failed to enable PACKET_FANOUT");
        return 1;
    }

    struct sockaddr_ll sa_ll;
    memset(&sa_ll, 0, sizeof(sa_ll));
    sa_ll.sll_family = AF_PACKET;
    sa_ll.sll_protocol = htons(ETH_P_ALL);
    sa_ll.sll_ifindex = ifr.ifr_ifindex;

    if (bind(sock_fd, (struct sockaddr *)&sa_ll, sizeof(sa_ll)) < 0) {
        perror("Failed to bind socket");
        return 1;
    }

    printf("eBPF program attached to interface %s\n", INTERFACE_NAME);
    printf("Listening for packets...\n");

    while (1) {
        struct timeval timestamp;
        ssize_t len = recv(sock_fd, packet, sizeof(packet), 0);
        if (len < 0) {
            perror("Failed to receive packet");
            break;
        }

        // Get timestamp
        if (ioctl(sock_fd, SIOCGSTAMP, &timestamp) < 0) {
            perror("Failed to get timestamp");
            break;
        }

        // Process packet here (calculate time span, etc.)
    }

    close(sock_fd);
    return 0;
}
