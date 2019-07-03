#include	"myftp.h"

// use ./myftpClient <port> <filename>
int main(int argc, char **argv)
{
	int socketfd;
	struct sockaddr_in servaddr,broadaddr;
	
	if(argc != 3)
	{
		printf("usage: ./myftpClient <port> <filename>\n");
		return 0;
	}
	
	socketfd = socket(AF_INET,SOCK_DGRAM,0);
    if(initCliAddr(socketfd, atoi(argv[1]), "255.255.255.255", &broadaddr))
        errCTL("initCliAddr");
        
    if(findServerAddr(socketfd, argv[2], &broadaddr, &servaddr))
        errCTL("findServerAddr");

    close(socketfd);
    
    if(startMyftpClient(&servaddr, argv[2]))
            errCTL("startMyftpClient");
	
	return 0;
}