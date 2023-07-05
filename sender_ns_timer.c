#include <stdio.h>	
#include <string.h>  
#include <stdlib.h>  
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>  
#include <time.h>

#define BUFLEN 512	
#define PORT 8888	
#define SLEEP_NS 1000000000UL // 1 second in nanoseconds

// SENDER

void die(char *s)
{
	perror(s);
	exit(1);
}

int main(void)
{
	struct sockaddr_in si_me, si_other; // si_other: sender
	int s, i, slen = sizeof(si_other), recv_len;
	char buf[BUFLEN];
	char send_buf[BUFLEN]; // Buffer for sending random numbers

	

	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		die("socket");
	}

	memset((char *)&si_me, 0, sizeof(si_me));

	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(s, (struct sockaddr *)&si_me, sizeof(si_me)) == -1)
	{
		die("bind");
	}

	//srand(time(NULL)); // Seed the random number generator

	//int random_num1 = rand();
	//int random_num2 = rand();

	int send_count = 0;

	while (1)
	{
		printf("Waiting for data...\n");
		fflush(stdout);

		if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_other, &slen)) == -1)
		{
			die("recvfrom()");
		}

		printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		printf("Data: %s\n", buf);

		// Prepare the buffer to send back
		snprintf(send_buf, sizeof(send_buf), "Send Count: %d", send_count);

		if (sendto(s, send_buf, strlen(send_buf), 0, (struct sockaddr *)&si_other, slen) == -1)
		{
			die("sendto()");
		}

		// Sleep for the specified frequency in nanoseconds until a new message is received
		struct timespec sleep_time;
        size_t time_ns = SLEEP_NS;
		sleep_time.tv_sec = time_ns/1000000000UL;
		sleep_time.tv_nsec = time_ns%1000000000UL;

		while (recvfrom(s, buf, BUFLEN, MSG_DONTWAIT, (struct sockaddr *)&si_other, &slen) == -1)
		{
		    printf("Sleep a bit...\n");

            //printf(sleep_time.tv_sec);
            //printf(sleep_time.tv_nsec);

			send_count += 1;

			snprintf(send_buf, sizeof(send_buf), "Send Count: %d", send_count);

			nanosleep(&sleep_time, NULL);
			
			if (sendto(s, send_buf, strlen(send_buf), 0, (struct sockaddr *)&si_other, slen) == -1)
			{
				die("sendto()");
			}
		}

	}

	close(s);
	return 0;
}

/*

1-(up to 1500) bytes -> usual limit for ethernet packetß

congirgurable interval and amount of information (number of bytes)

long option --<word>: 
./prog --dest 192.168.10.1 --interval 10000 --payload-size 250

short option:
./prog -i 10000 -p 250 -d 192.168.10.1


send packets with configurable interval and size - SENDER




*/
