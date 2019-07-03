#include "fill_packet.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>

extern pid_t pid;
void 
fill_iphdr ( struct ip *ip_hdr , const char* dst_ip)
{
/*
1. Header length = 28 bytes
2. Total length = 92 bytes
3. Id = 0
4. Flag = don’t fragment
5. TTL = 64
6. Protocol = icmp
7. Checksum (You can let OS do it for you.)
IP option uses 8 bytes, and no need to fill up the padding, just fill with ICMP header.
*/
	ip_hdr->ip_v	=(u_char)4;
	ip_hdr->ip_hl	=(u_char)(28/4);				/* u_char	header length */
	ip_hdr->ip_len	= htons(92);				/* short 	total length */
	ip_hdr->ip_id	=(0);				/* u_short	identification */
	ip_hdr->ip_off	=htons(IP_DF);			/* short 	fragment offset field */
	ip_hdr->ip_ttl	=(64);				/* u_char	time to live */
	ip_hdr->ip_p	=(1);				/* u_char	protocol , icmp 1 */
	ip_hdr->ip_sum	=(0);				/* u_short	checksum ,kernel fills in */
	/*dst_ip is like 140.117.15.12 a char string*/
	ip_hdr->ip_dst.s_addr = inet_addr(dst_ip);


	
}
int seq=1;
void
fill_icmphdr (struct icmphdr *icmp_hdr)
{
/*
1. Checksum (same as HW5)
2. ID: process id
3. Sequence number: Starting from 1, increase one by one.
4. Payload: Random, but don’t fill all 0. Note that data size must correspond with IP header.
*/

	
	icmp_hdr->type=(u_int8_t)(8);
	icmp_hdr->code=(u_int8_t)(0);
	icmp_hdr->un.echo.id=htons(pid);							//process id  pid
	//printf("%d\n",htons(pid));
	icmp_hdr->un.echo.sequence=htons(seq);
	icmp_hdr->checksum=0;
	icmp_hdr->checksum=(fill_cksum(icmp_hdr));				/*u_int16_t*/
	//printf("%x\n",fill_cksum(icmp_hdr));

	
}
/*
struct icmphdr
{
  u_int8_t type;			// message type 
  u_int8_t code;			// type sub-code 
  u_int16_t checksum;
  union
  {
    struct
    {
      u_int16_t	id;
      u_int16_t	sequence;
    } echo;					// echo datagram 
    u_int32_t	gateway;	// gateway address 
    struct
    {
      u_int16_t	__unused;
      u_int16_t	mtu;
    } frag;					// path mtu discovery 
  } un;
};
*/
u16
fill_cksum(struct icmphdr* icmp_hdr)
{
	unsigned short *addr = (unsigned short *)icmp_hdr;
	int len =sizeof(struct icmphdr)+ICMP_DATA_SIZE;

	int nleft = len;
	int sum = 0;
	unsigned short *w = addr;
	unsigned short answer = 0;
	
	while( nleft > 1 ) {
		sum += *w++;
		nleft -= 2;
	}
	
	if( nleft == 1 ) {
		*(unsigned char *) (&answer) = *(unsigned char *) w;
		sum += answer;
	}
	
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	answer = ~sum;
	return (answer);

}
/*

unsigned short in_cksum( unsigned short *addr, int len ) {
	int nleft = len;
	int sum = 0;
	unsigned short *w = addr;
	unsigned short answer = 0;
	
	while( nleft > 1 ) {
		sum += *w++;
		nleft -= 2;
	}
	
	if( nleft == 1 ) {
		*(unsigned char *) (&answer) = *(unsigned char *) w;
		sum += answer;
	}
	
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	answer = ~sum;
	return (answer);
}*/
