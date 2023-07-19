#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/filter.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <time.h>

#define MAX_PACKET_SIZE 4096
#define IP_ADDRESS "127.0.0.1" // Change this to the desired IP address to monitor

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
    int sock_fd;
    struct sockaddr_ll sa;
    char packet[MAX_PACKET_SIZE];

    // Create raw socket
    sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_fd < 0) {
        perror("Failed to open socket");
        return 1;
    }

    // Set packet filter based on IP address
    struct sock_filter ip_filter[] = {
        BPF_STMT(BPF_LD | BPF_H | BPF_ABS, 12),      // Load 16-bit IP destination address
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0x0000, 2, 0),   // Compare first 2 bytes of IP address
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0x0000, 1, 0),   // Compare last 2 bytes of IP address
        BPF_STMT(BPF_RET | BPF_K, (u_int32_t)-1),   // Return -1 (match)
        BPF_STMT(BPF_RET | BPF_K, 0),               // Return 0 (no match)
    };

    struct sock_fprog ip_filter_prog = {
        .len = sizeof(ip_filter) / sizeof(ip_filter[0]),
        .filter = ip_filter,
    };

    if (setsockopt(sock_fd, SOL_SOCKET, SO_ATTACH_FILTER, &ip_filter_prog, sizeof(ip_filter_prog)) < 0) {
        perror("Failed to set socket filter");
        return 1;
    }

    printf("Monitoring packets to IP address: %s\n", IP_ADDRESS);
    printf("Listening for packets...\n");

    struct timespec start, end;

    while (1) {
        ssize_t len = recv(sock_fd, packet, sizeof(packet), 0);
        if (len < 0) {
            perror("Failed to receive packet");
            break;
        }

        // Calculate time span
        clock_gettime(CLOCK_MONOTONIC, &end);
        size_t start_ticks = (size_t)(start.tv_sec * 1000000000UL) + start.tv_nsec;
        size_t end_ticks = (size_t)(end.tv_sec * 1000000000UL) + end.tv_nsec;
        size_t time_span_ticks = end_ticks - start_ticks;

        // Print time span
        printf("Packet received. Time span: %zu ticks\n", time_span_ticks);

        // Get the starting time for the next packet
        clock_gettime(CLOCK_MONOTONIC, &start);
    }

    close(sock_fd);
    return 0;
}
