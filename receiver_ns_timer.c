/*
Simple UDP client based on Silver Moon's tutorial
https://www.binarytides.com/programming-udp-sockets-c-linux/
*/
#include<stdio.h>	//printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include<unistd.h>  // close()

#define SERVER "127.0.0.1"
#define BUFLEN 512	//Max length of buffer
#define PORT 8888	//The port on which to send data

void die(char *s)
{
	perror(s);
	exit(1);
}

/*
flow of code: socket() -> sendto()/recvfrom()

*/

int main(void)
{
	struct sockaddr_in si_other;
	int s, i, slen=sizeof(si_other);
	char buf[BUFLEN];
	char message[BUFLEN];

	if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		die("socket");
	}

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT);
	
	if (inet_aton(SERVER , &si_other.sin_addr) == 0) 
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	
	printf("Enter message : ");
	gets(message); // ???
		
		//send the message
	if (sendto(s, message, strlen(message)+1 , 0 , (struct sockaddr *) &si_other, slen)==-1) // strlen +1 for string null terminator
	{
		die("sendto()");
	}
		
	while (1) {
		
        //receive a reply and print it
		//clear the buffer by filling null, it might have previously received data
		memset(buf,'\0', BUFLEN);
		//try to receive some data, this is a blocking call
		
		printf("Receving random numbers from server...\n");
		
		if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == -1)
		{
			die("recvfrom()");
		}
		
		puts(buf);
		
    }
	

	close(s);
	return 0;
}
