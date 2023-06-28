/*
Simple udp server based on Silver Moon's tutorial
https://www.binarytides.com/programming-udp-sockets-c-linux/

*/

#include<stdio.h>	//printf
#include<string.h>  //memset
#include<stdlib.h>  //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include<unistd.h>  // close()
#include<time.h>


// hold data being sent or received over a network socket
// acts as a temporary storage area for the data being transferred
#define BUFLEN 512	//Max length of buffer

#define PORT 8888	//The port on which to listen for incoming data, identifies a specific process

void die(char *s)
{
	perror(s);
	exit(1);
}

/*
flow of code: socket() -> bind() -> recvfrom() -> sendto()

*/
int main(void)
{
    // data structure representing an internet address
    /*
    definition of the sockaddr_in structure:
        struct sockaddr_in 
        {
            sa_family_t sin_family;  // Address family (typically AF_INET for IPv4 addresses)
            in_port_t sin_port;      // Port number in network byte order
            struct in_addr sin_addr; // IP address in network byte order
            char sin_zero[8];        // Padding to match the size of struct sockaddr
        };

    */
    struct sockaddr_in si_me, si_other; // si_other: sender
	
	int s, i, slen = sizeof(si_other) , recv_len;
	char buf[BUFLEN];
	char send_buf[BUFLEN]; // Buffer for sending random numbers
	
	//create a UDP(User Datagram Protocol) socket (datagram sockets; not connection oriented, unreliable communication); another type is TCP

    /*
    socket: serves as an endpoint for communication between processe/applications within a network

    socket(domain, type, protocol): taking 3 arguments to create a new socket:
        1. specifies the address family or domain of the socket; AF_INET for IPv4 addresses
        2. specifies the type of the socket; SOCK_DGRAM indicates a datagram socket
        3. specifies the protocol to be used; IPPROTO_UDP indicates that the UDP protocol will be used
    
    */
	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		die("socket");
	}

	
	// zero out the structure
	memset((char *) &si_me, 0, sizeof(si_me));
	
	si_me.sin_family = AF_INET;

    // convert to network byte order using htons()
        // convert a 16-bit unsigned short integer from host byte order
	si_me.sin_port = htons(PORT); 

    // sin_addr stores the IPv4 address. 
    // It is of type struct in_addr, which itself contains a single member s_addr representing the IP address in network byte order.
        // convert a 32-bit unsigned integer from host byte order
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    	
	// bind socket to port; binds a socket, specified by the socket descriptor s, to a specific address and port number.
    // bind the socket to the local address specified by the si_me struct 

    // This allows the socket to listen for incoming connections or send data from the specified local address and port.
	if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1) // ???
	{
		die("bind");
	}

	srand(time(NULL)); // Seed the random number generator


    //keep listening for data
    while(1)
	{
		printf("Waiting for data...\n");
		fflush(stdout);  // clear the output buffer ???
		
		//try to receive some data, this is a blocking call

        // recvfrom(): receive data from a socket. It reads data from the socket specified by the socket descriptor s and stores it in the buffer buf.

		if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1) // 0: optional flag - can modify recvfrom's behavior
		{
			die("recvfrom()");
		}
		
		//print details of the client/peer and the data received

        /*
            inet_ntoa(): converts an IPv4 address from binary form to a human-readable string format

            ntohs(): convert a 16-bit unsigned short integer from network byte order to host byte order
        */
		printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		printf("Data: %s\n" , buf);


		// Generate random numbers 
		int random_num1 = rand();
		int random_num2 = rand();

		// DEBUG:
		// printf("Random Numbers: %d, %d\n", random_num1, random_num2);

		// Prepare the buffer to send back
		snprintf(send_buf, sizeof(send_buf), "Random Numbers: %d, %d", random_num1, random_num2);


		
		//now reply the client with the same data
		if (sendto(s, send_buf, strlen(send_buf), 0, (struct sockaddr*) &si_other, slen) == -1)
		{
			die("sendto()");
		}
	}

	close(s);
	return 0;



}

/*

run server:

$ gcc simple_UDP_server.c -o simple_UDP_server
$ ./simple_UDP_server

Waiting for data...


*/