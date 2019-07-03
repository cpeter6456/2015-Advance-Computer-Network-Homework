#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include "fill_packet.h"
#include "pcap.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
pid_t pid;
char *targetip;
extern int seq;
int getmyip(){
	targetip = malloc(64);
	 struct ifaddrs *ifaddr, *ifa;
   int family, s;
   char host[NI_MAXHOST];

   if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
   }

   for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) {
                s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                                               host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                if (s != 0) {
                        printf("getnameinfo() failed: %s\n", gai_strerror(s));
                        exit(EXIT_FAILURE);
                }
                printf("<Interface>: %s \t <Address> %s\n", ifa->ifa_name, host);
                
        }
   }
   return 0;
}
int main(int argc, char* argv[])
{
	getmyip();
	/**
	argv[0] = ./myping
	argv[1] = -g
	argv[2] = 140.117.171.140	//gatway
	argv[3] = 140.117.171.171
	argv[4] = -c
	argv[5] = 8
	argv[6] = -w
	argv[7] = 1500
	**/


	strcpy(targetip,argv[3]);
	int sockfd;
	int on = 1;
	printf("(int)sizeof(struct ip)=%d\n",(int)sizeof(struct ip));
	printf("(int)sizeof(struct icmphdr)=%d\n",(int)sizeof(struct icmphdr));
	pid = getpid();
	struct sockaddr_in dst;
	myicmp *packet = (myicmp*)malloc(PACKET_SIZE);
	memset(packet,0,PACKET_SIZE);
	int count = DEFAULT_SEND_COUNT;
	int timeout = DEFAULT_TIMEOUT;

	if(argc==4 ){
		count = DEFAULT_SEND_COUNT;
		timeout = DEFAULT_TIMEOUT;
	}
	else if(argc==8 ){
		count = atoi(argv[5]);
		timeout = atoi(argv[7]);
	}else{
		printf("format error\n");
	}
	/* 
	 * in pcap.c, initialize the pcap
	 */
	pcap_init( argv[3] , timeout);

	if((sockfd = socket(AF_INET, SOCK_RAW , IPPROTO_RAW)) < 0)
	{
		perror("socket");
		exit(1);
	}

	if(setsockopt( sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
	{
		perror("setsockopt");
		exit(1);
	}
	printf("ping %s (data size =%u,pid=%d,timeout=%d ms,count=%d)\n",argv[3],56,pid,timeout,count);
	dst.sin_family = AF_INET;
	
	//data size =%u,pid=%d 56,pid,
	 

	fill_iphdr ( &(packet->ip_hdr) , argv[2]);
	dst.sin_addr = packet->ip_hdr.ip_dst;

	packet->ip_option[0]=(IPOPT_LSRR);	/* option Type */
	packet->ip_option[1]=(1+1+1+4);		/* option Length */
	packet->ip_option[2]=(4);			/* option ptr source routing 每次hop後都會+4 */

	struct in_addr des;
	des.s_addr = inet_addr(argv[3]);
	memcpy( &(packet->ip_option[3]) ,&(des),4);/* option Data */
	
    

	/*
	 *   Use "sendto" to send packets, and use "pcap_get_reply"(in pcap.c) 
 	 *   to get the "ICMP echo response" packets.
	 *	 You should reset the timer every time before you send a packet.
	 */

	 int i=0;
	 for(i=0;i<count;i++){
	 	memset(packet->data,'H',ICMP_DATA_SIZE);
	 	fill_icmphdr ( &(packet->icmp_hdr));//for seq++
	 	/*Payload*/

	 	if(sendto(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr *)&dst, sizeof(dst)) < 0)
		{
				perror("sendto");
				exit(1);
		}

		pcap_get_reply(argv[2],argv[3]);
		seq++;
	}
	free(packet);

	return 0;
}


