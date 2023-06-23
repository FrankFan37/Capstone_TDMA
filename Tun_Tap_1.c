#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>


#define BUFFER_SIZE 2048

int tap_fd;
int raw_fd;

const char* dev_name;

// Create a TUN/TAP device

// TUN interfaces (IFF_TUN) transport layer 3 (L3) Protocol Data Units (PDUs)
// TAP interfaces (IFF_TUN) transport layer 2 (L2) PDUs

int create_tap_device(char *dev_name, int flags) {
    struct ifreq ifr;
    int err;

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

// Read packets from the TUN/TAP device
void read_packets(int tap_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t nread;

    while (1) {
        nread = read(tap_fd, buffer, sizeof(buffer));
        if (nread < 0) {
            perror("read");
            break;
        }

        printf("Received packet: %lu\n", nread);
        for( int i=0; i<nread; i++) {
          printf("%02X ", buffer[i]);
        }
        printf("\n");
        
        struct ethhdr *eth = (struct ethhdr *)(buffer);
        printf("DST: %02X:%02X:%02X:%02X:%02X:%02X\n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);
        
        printf("SRC: %02X:%02X:%02X:%02X:%02X:%02X\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);
        
    }
}

// Write packets to the TUN/TAP device
void write_packets(int tap_fd, const char *packet, size_t packet_len) {
    ssize_t nwritten = write(tap_fd, packet, packet_len);
    if (nwritten < 0) {
        perror("write");
    } else {
        printf("Sent packet: %.*s\n", (int)nwritten, packet);
    }
}

// function 1: 
// input: read from device: read name of device; output: mask & IP

void get_interface_details(const char *interface_name, char *ip_address, char *netmask_address) {
    int sockfd;
    struct ifreq ifr;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return;
    }

    // Set the interface name
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);

    // Get the IP address

    // ioctl: input and output control, talk to device drivers 
    if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
        perror("ioctl (SIOCGIFADDR)");
        close(sockfd);
        return;
    }

    struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
    // sin_addr: the IP address in the socket
    inet_ntop(AF_INET, &(addr->sin_addr), ip_address, INET_ADDRSTRLEN); // get

    // Get the netmask
    if (ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0) {
        perror("ioctl (SIOCGIFNETMASK)");
        close(sockfd);
        return;
    }

    struct sockaddr_in *netmask = (struct sockaddr_in *)&ifr.ifr_netmask; // ifr.ifr_netmask ???
    inet_ntop(AF_INET, &(netmask->sin_addr), netmask_address, INET_ADDRSTRLEN);

    // Close the socket
    close(sockfd);
}

// function 2: grasp MAC address (physical address)

void get_mac_address(const char *interface_name, char *mac_address) {
    int sockfd;
    struct ifreq ifr;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return;
    }

    // Set the interface name
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);

    // Get the MAC address
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl (SIOCGIFHWADDR)");
        close(sockfd);
        return;
    }

    unsigned char *mac = (unsigned char *)ifr.ifr_hwaddr.sa_data; // ifr.ifr_hwaddr ???
    // the MAC address is formatted as a string in the format XX:XX:XX:XX:XX:XX and stored in the mac_address buffer.
    sprintf(mac_address, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    

    // Close the socket
    close(sockfd);
}



// function 3: modify the IP and mask of "tun0"
// input: tun0, ip, mask; output: null

void modify_interface_details(const char *interface_name, const char *new_ip_address, const char *new_netmask) {
    int sockfd;
    struct ifreq ifr;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return;
    }

    // Set the interface name
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);

    // Get the current flags of the interface
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
        perror("ioctl (SIOCGIFFLAGS)");
        close(sockfd);
        return;
    }

    // Disable the interface before modifying its address
    ifr.ifr_flags &= ~IFF_UP;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
        perror("ioctl (SIOCSIFFLAGS)");
        close(sockfd);
        return;
    }

    // Set the new IP address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    
    // int inet_pton(int af, const char * src, void * dst);
    inet_pton(AF_INET, new_ip_address, &(addr.sin_addr));

    // void *memcpy(void *dest, const void * src, size_t n)
    memcpy(&ifr.ifr_addr, &addr, sizeof(struct sockaddr));

    if (ioctl(sockfd, SIOCSIFADDR, &ifr) < 0) {
        perror("ioctl (SIOCSIFADDR)");
        close(sockfd);
        return;
    }

    // Set the new netmask
    struct sockaddr_in netmask;
    memset(&netmask, 0, sizeof(netmask));
    netmask.sin_family = AF_INET;
    inet_pton(AF_INET, new_netmask, &(netmask.sin_addr));
    memcpy(&ifr.ifr_netmask, &netmask, sizeof(struct sockaddr)); // ifr.ifr_netmask ???

    if (ioctl(sockfd, SIOCSIFNETMASK, &ifr) < 0) {
        perror("ioctl (SIOCSIFNETMASK)");
        close(sockfd);
        return;
    }

    // Enable the interface
    ifr.ifr_flags |= IFF_UP;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
        perror("ioctl (SIOCSIFFLAGS)");
        close(sockfd);
        return;
    }

    // Close the socket
    close(sockfd);
}

// function 4: delete IP address and netmask of "enp0s1" interface

/*
void delete_interface_details(const char *interface_name) {
    int sockfd;
    struct ifreq ifr;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return;
    }

    // Set the interface name
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);

    // Delete the IP address and netmask
    if (ioctl(sockfd, SIOCDIFADDR, &ifr) < 0) {
        perror("ioctl (SIOCDIFADDR)");
        close(sockfd);
        return;
    }

    // Close the socket
    close(sockfd);
}
*/

// ERROR: ioctl (SIOCSIFNETMASK): Cannot assign requested address

void delete_interface_details(const char *interface_name) {
    int sockfd;
    struct ifreq ifr;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return;
    }

    // Set the interface name
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);

    // Delete the IP address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "", &(addr.sin_addr));
    memcpy(&ifr.ifr_addr, &addr, sizeof(struct sockaddr));

    if (ioctl(sockfd, SIOCSIFADDR, &ifr) < 0) {
        perror("ioctl (SIOCSIFADDR)");
        close(sockfd);
        return;
    }

    // Delete the netmask
    struct sockaddr_in netmask;
    memset(&netmask, 0, sizeof(netmask));
    netmask.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &(netmask.sin_addr));
    memcpy(&ifr.ifr_netmask, &netmask, sizeof(struct sockaddr));

    if (ioctl(sockfd, SIOCSIFNETMASK, &ifr) < 0) {
        perror("ioctl (SIOCSIFNETMASK)");
        close(sockfd);
        return;
    }

   /*
   // change signals the system to remove the netmask associated with the interface.
   ifr.ifr_netmask.sa_family = AF_UNSPEC;
    if (ioctl(sockfd, SIOCSIFNETMASK, &ifr) < 0) {
        perror("ioctl (SIOCSIFNETMASK)");
        close(sockfd);
        return;
    */

    // Close the socket
    close(sockfd);
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

// -----------------------------------
// multi-threads
// communication between raw socket and "tap0"

void* receive_from_raw_socket(void* arg) {
    char buffer[BUFFER_SIZE];
    ssize_t nread;

    while (1) {
        // nread = recv(raw_fd, buffer, sizeof(buffer), 0);

        struct sockaddr_ll sa;
        socklen_t sa_len = sizeof(struct sockaddr_ll);
        nread = recvfrom(raw_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&sa, &sa_len);
        
        if (nread < 0) {
            perror("recv");
            continue;
            //break;
        }

        // Write the received packet to the tap interface
        ssize_t nwritten = write(tap_fd, buffer, nread);   // ???
        if (nwritten < 0) {
            perror("write error");
            //break;
            continue;
        }
        printf("Receiving from dev -> tap\n");
    }

    printf("Leaving receive thread\n")

    pthread_exit(NULL);
}

void* read_from_tap_interface(void* arg) {
    char buffer[BUFFER_SIZE];
    ssize_t nread;

    struct ifreq if_index={
        .ifr_name={}
    };
    strcpy( if_index.ifr_name, dev_name );
    if(ioctl(raw_fd, SIOCGIFINDEX, &if_index) < 0) {
	    perror("ioctl get if index");
	    return 0;
    }

    while (1) {
        nread = read(tap_fd, buffer, sizeof(buffer));
        if (nread < 0) {
            perror("read");
            //break;
            continue;
        }

        // Send the received packet to the raw socket

        // ssize_t nwritten = send(raw_fd, buffer, nread, 0); 

        struct ethhdr *eth = (struct ethhdr *)buffer;
        
        struct sockaddr_ll sa = {
            .sll_ifindex = if_index.ifr_ifindex,
            .sll_halen = ETH_ALEN,
            .sll_addr[0] = eth->h_dest[0],
            .sll_addr[1] = eth->h_dest[1],
            .sll_addr[2] = eth->h_dest[2],
            .sll_addr[3] = eth->h_dest[3],
            .sll_addr[4] = eth->h_dest[4],
            .sll_addr[5] = eth->h_dest[5],
        };

        ssize_t nwritten = sendto(raw_fd, buffer, nread, 0, (struct sockaddr*) &sa, sizeof(sa));

        if (nwritten < 0) {
            perror("send");
            //break;
            continue;
        }
        printf("Sending from tap -> dev\n");

    }

    printf("Leaving send hread\n");

    //close(raw_fd);	
    //close(tap_fd);

    pthread_exit(NULL);
}









// Entry point of the program
int main() {

    if (argc<2) {
	    printf("No device was specified\n");
	    return -1;
    }
    dev_name = argv[1];
    
    char tap_name[IFNAMSIZ] = {0};  // Initialize tap_name

    if (geteuid() != 0) {
        fprintf(stderr, "You need root privileges to run this program.\n");
        return 1;
    }

    // tap_fd = create_tap_device(tap_name, IFF_TAP);  // Use IFF_TUN for TUN device

    struct ifreq ifr;
    int err;

    if ((tap_fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("open");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));

    // ifr_flags field to choose whether to create a TUN (i.e. IP) or a TAP (i.e. Ethernet)
    ifr.ifr_flags = IFF_TAP;

    // ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    if (tap_name!= NULL) {
        strncpy(ifr.ifr_name, tap_name, IFNAMSIZ);
    }
    if ((err = ioctl(tap_fd, TUNSETIFF, (void *)&ifr)) < 0) {
        perror("ioctl");
        close(tap_fd);
        return err;
    }

    strcpy(tap_name, ifr.ifr_name);

    // ------------------------

    if (tap_fd < 0) {
        fprintf(stderr, "Failed to create TUN/TAP device.\n");
        return 1;
    }

    printf("TUN/TAP device %s created.\n", tap_name);

    /*
    write() sends a (single) packet or frame to the virtual network interface;
    read() receives a (single) packet or frame from the virtual network interface;
    */

    // write_packets(tun_fd, "Hello, TUN/TAP!", 15);


    // added part for grasping the ip address and netmask...
    // ------------------------------------------
    char ip_address[INET_ADDRSTRLEN];
    char netmask_address[INET_ADDRSTRLEN];

    get_interface_details(dev_name, ip_address, netmask_address);

    printf("IP Address: %s\n", ip_address);
    printf("Netmask: %s\n", netmask_address);

    // ------------------------------------

    // Allocate a buffer to store the MAC address
    char mac_address[18];  // XX:XX:XX:XX:XX:XX + null terminator included -> 17+1
    get_mac_address(dev_name, mac_address);

    printf("MAC Address: %s\n", mac_address);
    // -----------------------------------

    modify_interface_details("tap0", ip_address, netmask_address);

    // -----------------------------------
    
    if (modify_mac_address("tap0", mac_address) == 0) {
        printf("Modified MAC address for tap0\n");
    } else {
        fprintf(stderr, "Failed to modify MAC address for tap0\n");
    }

    // ------------------------------------
   
    const char *interface_name = dev_name;

    delete_interface_details(interface_name);

    printf("Deleted interface details for %s\n", interface_name);

    // -----------------------------------
    // multi-threads communication between raw socket and tap0

    pthread_t thread1, thread2;

    // Create the raw socket
    raw_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (raw_fd < 0) {
        perror("socket (raw)");
        close(tap_fd);
        return 1;
    }

    struct sockaddr_ll sa;
    memset(&sa, 0, sizeof(struct sockaddr_ll));
    sa.sll_family = AF_PACKET;
    sa.sll_protocol = htons(ETH_P_ALL);

    // sa.sll_ifindex = ifr.ifr_ifindex; // index of interface
    sa.sll_ifindex = if_nametoindex(ifr.ifr_name);
    
    if (bind(raw_fd, (struct sockaddr *)&sa, sizeof(struct sockaddr_ll)) < 0) {
        perror("bind");
        close(raw_fd);
        close(tap_fd);
        return 1;
    }

    if (setsockopt(raw_fd, SOL_SOCKET, SO_BINDTODEVICE, dev_name, strlen(dev_name)+1)<0) {
	    perror("setsockopt BIND TO DEVICE");
    }

    // Create thread 1 to receive from raw socket and write to tap interface
    if (pthread_create(&thread1, NULL, receive_from_raw_socket, NULL) != 0) {
        perror("pthread_create (thread1)");
        close(raw_fd);
        close(tap_fd);
        return 1;
    }

    // Create thread 2 to read from tap interface and send to raw socket
    if (pthread_create(&thread2, NULL, read_from_tap_interface, NULL) != 0) {
        perror("pthread_create (thread2)");
        close(raw_fd);
        close(tap_fd);
        return 1;
    }

    // Wait for both threads to complete
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    close(raw_fd);
    close(tap_fd);


    printf("\nProgram terminated.\n");
    //modify_interface_details(dev_name, ip_address, netmask_address);

    return 0;
}


/*
Run this program:

gcc Tun_Tap_1.c -o Tun_Tap_1
sudo ./Tun_Tap_1

gcc test.c -o test
sudo ./test
*/

/*
reset environment:

sudo ifconfig enp0s1 down
sudo ifconfig enp0s1 up
*/

/*
find out how to grab value MAC/IP address...

assign them to tap device ethernet packets ready to go

create functions:
1. input: read from device: read name of device; output: mask & IP
2. same input; output: mac address (reference in C)

3. input: tun0, ip, mask; output: null

4. delete IP/mask addresses

-------------------

make it a Tun0 -> Tap

5. "Tap0", MAC addr 
*/

/*
get Linux IP address terminal: ifconfig

    IP addr -> inet
    mask -> netmask
    MAC addr -> ether

------------------------
'enp0s1'

the name consists of the following parts:

en: This prefix indicates an Ethernet interface.
p0: This represents the slot number or the location of the interface on the motherboard or expansion card.
s1: This represents the port or socket number.

--------------------


create a new raw socket and read from tap0 send to raw sockets, 
multiple threads write/receive tap0

while loop wait message then write 

thread 1: receive from raw, write to tap - blocking operation
thread 2: read from tap, send to raw socket 

raw socket - real device

MAC address - same as in real device

*/

