#include "pcap.h"
#include <sys/types.h>
#include <pcap/pcap.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>


extern pid_t pid;
extern u16 icmp_req;
extern char *targetip;
static const char* dev = "eth0";
static char* net;
static char* mask;

static char filter_string[FILTER_STRING_SIZE] = "";

static pcap_t *p;
static struct pcap_pkthdr hdr;

/*
 * This function is almost completed.
 * But you still need to edit the filter string.
 */
void pcap_init( const char* dst_ip ,int timeout )
{	
	int ret;
	char errbuf[PCAP_ERRBUF_SIZE];
	
	bpf_u_int32 netp;
	bpf_u_int32 maskp;
	
	struct in_addr addr;
	
	struct bpf_program fcode;
	
	ret = pcap_lookupnet(dev, &netp, &maskp, errbuf);
	if(ret == -1){
		fprintf(stderr,"%s\n",errbuf);
		exit(1);
	}
	
	addr.s_addr = netp;
	net = inet_ntoa(addr);	
	if(net == NULL){
		perror("inet_ntoa");
		exit(1);
	}
	
	addr.s_addr = maskp;
	mask = inet_ntoa(addr);
	if(mask == NULL){
		perror("inet_ntoa");
		exit(1);
	}
	
	
	p = pcap_open_live(dev, 8000, 1, timeout, errbuf);
	if(!p){
		fprintf(stderr,"%s\n",errbuf);
		exit(1);
	}
	
	/*
	 *    you should complete your filter string before pcap_compile
	 */
	strcpy(filter_string,"src host ");
	strncat(filter_string,targetip,strlen(targetip));
	strcat(filter_string," and icmp and icmp[icmptype] == 0");
	//strcpy(filter_string,"src host 192.168.79.136 and icmp");
	printf("%s\n",filter_string);
	//src host 192.168.79.136,
	
	if(pcap_compile(p, &fcode, filter_string, 0, maskp) == -1){
		pcap_perror(p,"pcap_compile");
		exit(1);
	}
	
	if(pcap_setfilter(p, &fcode) == -1){
		pcap_perror(p,"pcap_setfilter");
		exit(1);
	}
}

int pcap_get_reply(  const char* router_ip, const char* dst_ip )
{
	
	struct timeval tv;
    struct timezone tz;
    gettimeofday (&tv, &tz);
	/*gettimeofday()會把目前的時間有tv 所指的結構返回,當地時區的信息則放到tz 所指的結構中。*/
	const u_char *ptr;

	ptr = pcap_next(p, &hdr);
	/*hdr is packet information,returns a u_char pointer to the data in that packet*/
	
	/*
	 * google "pcap_next" to get more information
	 * and check the packet that ptr pointed to.
	 */
	if(ptr == NULL)  
    {
    	/*
		returns  NULL  if  an error occurred, or if no packets were read from a
       live capture 
    	*/
        printf("Reply from %s = *\n",dst_ip);  
        return -1;
    }  //1 sec = 10^6 usec
   
    printf("Reply from %s = %.3f\n",dst_ip,
    		(float)((hdr.ts.tv_sec-tv.tv_sec)+(hdr.ts.tv_usec-tv.tv_usec)/1000000.0));
    printf("\tRouter:%s\n",router_ip);    
   
	
//sprintf(filter_string,"src host %s and icmp[icmptype] == 0 and icmp[4:2] == %d and icmp[6:2] == %hu",dst_ip,(int)pid,icmp_req);
	
	return 0;
}