#include	"myftp.h"
int main(int argc,char **argv)
{
    int socketfd;
    FILE *file;
    struct stat buf;
    struct sockaddr_in servaddr, clientaddr;
    struct in_addr NoUse;
    char device[DEVICELEN];
    char MyIP[128];
    int tempPort;

    if(argc != 3)
    {
        printf("usage: ./myftpServer <port> <filename>\n");
        return 0;
    }

    if(lstat(argv[2], &buf) < 0) 
    {
        printf("unknow file : %s\n", argv[2]);
        return 0;
    }

    socketfd = socket(AF_INET,SOCK_DGRAM,0);

    if( getIFname( socketfd, device ,MyIP,&NoUse) )
        errCTL("getIFname");
    debugf("network interface = %s\n",device);
    debugf("network port = %d\n",atoi(argv[1]));

    if(initServAddr(socketfd, atoi(argv[1]), device, &servaddr))
        errCTL("initServAddr");

    //Function: Server can serve multiple clients
    //Hint: Use loop, listenClient(), startMyFtpServer(), and ( fork() or thread ) 
    printf("Myftp Server Start!!\nWait clients\n");
    printf("Share file: %s\n",argv[2]);
    int ListenBroadcastPort = atoi(argv[1]);
    
    
    
    while(1){
        if (listenClient(socketfd,ListenBroadcastPort,&tempPort,argv[2], &clientaddr,MyIP)!=-1) {
            
            pid_t pid = fork();
            if(pid == 0){
                startMyftpServer(tempPort,&clientaddr, argv[2]);
                break;
            }
        }
    }

    return 0;
}