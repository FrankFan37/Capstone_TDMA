#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <pthread.h>
#include <netinet/in.h>
#include <linux/if_packet.h>

#include <signal.h>

#include <tap.h>
#include <net_management.h>

#define BUFFER_SIZE 2048

int tap_fd;
int raw_fd;
const char *dev_name;


static int terminate = 0;






// -----------------------------------
// multi-threads
// communication between raw socket and "tap0"
void* receive_from_raw_socket(void* arg) {
    char buffer[BUFFER_SIZE];
    ssize_t nread;

    while (!terminate) {
        // nread = recv(raw_fd, buffer, sizeof(buffer), 0);

        struct sockaddr_ll sa;
        socklen_t sa_len = sizeof(struct sockaddr_ll);
        nread = recvfrom(raw_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&sa, &sa_len);

        if (nread < 0) {
            perror("recv");
	        continue;
        }

        // Write the received packet to the tap interface
        ssize_t nwritten = write(tap_fd, buffer, nread);   // ???
        if (nwritten < 0) {
            perror("write error");
            continue;
        }
	// printf("Receiving from dev -> tap\n");
    }

    printf("Leaving receive thread\n");

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

    while (!terminate) {
        nread = read(tap_fd, buffer, sizeof(buffer));
        if (nread < 0) {
            perror("read");
            break;
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

        ssize_t nwritten = sendto(raw_fd, buffer, nread, 0, (struct sockaddr *) &sa, sizeof(sa));

        if (nwritten < 0) {
            perror("send");
            break;
        }
	// printf("Sending from tap -> dev\n");
    }

    printf("Leaving send hread\n");
    pthread_exit(NULL);
}





void cleanup(int signal_number) {
    if(signal_number == SIGINT) {
        terminate = 1;
        signal(SIGINT, SIG_DFL);
        printf("Leaving gracefuly\n");

    }
}



// Entry point of the program
int main(int argc, char **argv) {

    if ( argc<2 ) {
	    printf("No device was specified\n");
	    return -1;
    }
    dev_name = argv[1];


    signal(SIGINT, cleanup);

    char tap_name[IFNAMSIZ] = {0};  // Initialize tap_name

    if (geteuid() != 0) {
        fprintf(stderr, "You need root privileges to run this program.\n");
        return 1;
    }

    tap_fd = create_tap_device(tap_name);  // Use IFF_TUN for TUN device



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
    sa.sll_ifindex = if_nametoindex(tap_name);

/*    if (bind(raw_fd, (struct sockaddr *)&sa, sizeof(struct sockaddr_ll)) < 0) {
        perror("bind");
        close(raw_fd);
        close(tap_fd);
        return 1;
    }
*/

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
    modify_interface_details(dev_name, ip_address, netmask_address);

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

