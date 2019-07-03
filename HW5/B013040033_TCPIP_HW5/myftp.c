#include "myftp.h"

int getIFname( int socketfd, char *device ,char*MyIP,struct in_addr*broadcastAddr) {
    memset(device, 0,sizeof(char)*DEVICELEN);
    //Function: To get the device name
    //Hint:     Use ioctl() with SIOCGIFCONF as an arguement to get the interface list
    struct ifconf ifconf;
    struct ifreq ifr[10];
    
    struct in_addr MyMask,MyIPaddr;
    //Point ifconf's ifc_buf(ifc_buf is onlychar*) to array , and set len it can use
    ifconf.ifc_buf = (char *) ifr;
    ifconf.ifc_len = sizeof(struct ifreq)*10;
    if (!ioctl (socketfd, SIOCGIFCONF, (char *) &ifconf)){
        

        int intrface = ifconf.ifc_len /sizeof (struct ifreq);
        printf("interface num is intrface=%d\n\n",intrface);

        int i=0;
        for (i=0;i<intrface; i++) {  
        
            printf("ifr[%d].ifr_name = [%s]\n" , i, ifr[i].ifr_name);  
            printf("local addr = [%s]\n" ,inet_ntoa(((struct sockaddr_in*)&(ifr[i].ifr_addr))->sin_addr));
            
            
            if(strcmp("lo",ifr[i].ifr_name) != 0){
                
                memset(device, 0,sizeof(char)*DEVICELEN);
                strcpy(device,ifr[i].ifr_name);
                
                memset(MyIP,0,128);
                
                strcpy(MyIP,inet_ntoa(((struct sockaddr_in*)&(ifr[i].ifr_addr))->sin_addr));
                MyIPaddr = ((struct sockaddr_in*)&(ifr[i].ifr_addr))->sin_addr;
                printf("Put device = %s\n",device);
            }
            
        } 
        /* get network mask of my interface */
        struct ifreq ifreq;strcpy(ifreq.ifr_name,device);
        if (ioctl(socketfd, SIOCGIFNETMASK,&ifreq) < 0) {
            perror("ioctl SIOCGIFNETMASK error");
            MyMask.s_addr = 0;
        }
        else {
            struct sockaddr_in *sin_ptr = (struct sockaddr_in *)&ifreq.ifr_addr;
            MyMask = sin_ptr->sin_addr;
            printf("MyMask = %s\n",inet_ntoa(MyMask));
            (*broadcastAddr).s_addr = MyIPaddr.s_addr | (~MyMask.s_addr);
            printf("broadcastAddr = %s\n",inet_ntoa((*broadcastAddr)));

        }
        printf("MyIP = %s\n",MyIP);
        printf("Last Put device = %s\n",device);
     }else{
        errCTL("getIFname::ioctl::SIOCGIFCONF");
     }
    return 0;
}

int initServAddr( int socketfd, int port, const char *device, struct sockaddr_in *server_addrIpv4 ) {
    //Function: Bind device with socketfd
    //          Set sever address(struct sockaddr_in), and bind with socketfd
    //Hint:     Use setsockopt to bind the device
    //          Use bind to bind the server address(struct sockaddr_in)

    setsockopt(socketfd, SOL_SOCKET, SO_BINDTODEVICE, device, strlen(device));

    //SO_REUSEADDR，打開或關閉地址复用功能。
    //當option_value不等於0時，打開，否則，關閉。
    int sock_opt = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (void*)&sock_opt, sizeof(sock_opt) ) == -1){
        return -1;
    }

    struct sockaddr_in ListenBroadcast;
    memset(&ListenBroadcast,0,sizeof(struct sockaddr_in));
    ListenBroadcast.sin_family=AF_INET;
    ListenBroadcast.sin_port= htons(port);
    ListenBroadcast.sin_addr.s_addr=htonl(INADDR_ANY);
    if(bind(socketfd, (struct sockaddr *)(&ListenBroadcast), sizeof(struct sockaddr_in)) < 0){
        printf("Bind error!\n");
        exit(1);
    }


    return 0;
}
int initCliAddr( int socketfd, int BroadcastPort, char *sendClient, struct sockaddr_in *broadcastAddr ) {
    //Function: Set socketfd with broadcast option and the broadcast address(struct sockaddr_in)
    //Hint:     Use setsockopt to set broadcast option
    

    //SO_BROADCAST，允許或禁止發送廣播數據。
    //將option_value不等於0時，允許，否則，禁止。
    int sock_opt = 1;
    if(setsockopt(socketfd, SOL_SOCKET, SO_BROADCAST, (void*)&sock_opt,sizeof(sock_opt))<0){
        perror("initCliAddr::setsockopt::");exit(1);
    }


    memset(broadcastAddr, 0, sizeof(struct sockaddr_in));   /* Zero out structure */
    (*broadcastAddr).sin_family = AF_INET;                 /* Internet address family */
    (*broadcastAddr).sin_port = htons(BroadcastPort); 

    //?? I guess sendClient is Broadcast IP address ? 

    //ERROR inet_addr return error at 255.255.255.255 , use inet_aton
    //(*broadcastAddr).sin_addr.s_addr = inet_addr(*sendClient);  
    //(*broadcastAddr).sin_addr.s_addr=INADDR_BROADCAST; 
    
     /*if(inet_aton( sendClient,&((*broadcastAddr).sin_addr) )< 0){
        perror("initCliAddr::iner_aton::");
        exit(1);
    } */

    //*******I don't know why use 255.255.255.255 will unreachable
    //*******Use getIFname::(*broadcastAddr).s_addr = MyIPaddr.s_addr | (~MyMask.s_addr);
    char device[DEVICELEN];
    char MyIP[128];
    struct in_addr AbroadcastAddr;
    getIFname(socketfd, device ,MyIP,&AbroadcastAddr);
    (*broadcastAddr).sin_addr=AbroadcastAddr;
   
    printf("broadcastAddr = %s\n",inet_ntoa((*broadcastAddr).sin_addr));

    return 0;
    
}

int findServerAddr( int socketfd, char *filename, const struct sockaddr_in *broadcastAddr, struct sockaddr_in *servaddr ) {
    //Function: Send broadcast message to find server
    //          Set timeout to wait for server replay
    //Hint:     Use struct bootServerInfo as boradcast message
    //          Use setsockopt to set timeout
    //SO_SNDTIMEO,SO_RCVTIMEO

    printf("findServerAddr\n");
    struct bootServerInfo packet;
    struct timeval timeout;      
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    
    if (setsockopt (socketfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(struct timeval)) < 0){
        perror("setsockopt SO_RCVTIMEO failed\n");exit(1);    
    }

    strcpy(packet.filename,filename);


    int SendBytes = sendto(socketfd, &packet,sizeof(packet), 0, 
        (struct sockaddr *)broadcastAddr,sizeof(struct sockaddr_in));
    if(SendBytes<0){perror("findServerAddr::sendto");exit(1);}

    memset(&packet, 0,sizeof(packet)); 
    int recvLen=sizeof(struct sockaddr_in);
    int RecBytes  = recvfrom(socketfd, &packet,sizeof(packet), 0 ,
        (struct sockaddr *)servaddr, &recvLen);
    int TimeoutNum=0;
    while(RecBytes<0){
        printf("[findServerAddr]::Send Broadcast Again\n");

        if(TimeoutNum>10){
            printf("[findServerAddr Timeout >20sec]::Client exit\n");
            exit(1);
        }

        TimeoutNum++;

        SendBytes = sendto(socketfd, &packet,sizeof(packet), 0, 
        (struct sockaddr *)broadcastAddr,sizeof(struct sockaddr_in));
        if(SendBytes<0){perror("findServerAddr::sendto2");exit(1);}

        memset(&packet, 0,sizeof(packet)); 
        RecBytes  = recvfrom(socketfd, &packet,sizeof(packet), 0 ,
        (struct sockaddr *)servaddr, &recvLen);
    }
    printf("servaddr->sin_port=%d , ervaddr->sin_addr=%s",ntohs(servaddr->sin_port),inet_ntoa(servaddr->sin_addr));
    
    printf("[Receive Reply]\n\tGetMyFTPserver addr:%s\n\tGetMyFTPserver TempConnectPort %d\n",
        packet.servAddr,packet.connectPort);
    
    //Update port for transmit data
    servaddr->sin_port = htons(packet.connectPort);
    printf("%d %s",ntohs(servaddr->sin_port),inet_ntoa(servaddr->sin_addr));

    return 0;
}
int listenClient(int socketfd, int port, int *tempPort, char *serFilename, struct sockaddr_in *clientaddr,char* MyIP){


    srand (time(NULL));
    struct bootServerInfo packet;
    socklen_t recvLen=sizeof(struct sockaddr_in);
    memset(clientaddr, 0,recvLen);

    int RecBytes  = recvfrom(socketfd, &packet,sizeof(struct bootServerInfo), 0 ,
        (struct sockaddr *)clientaddr, &recvLen);

    if(RecBytes<0){perror("listenClient::recvfrom");exit(1);}

    printf("[Receive Request]\n\tFile:%s",packet.filename);
    printf("\n\tClient's IP address %s\n\tPort is: %d\n",
        inet_ntoa( (*clientaddr).sin_addr),(int) ntohs((*clientaddr).sin_port));

    if(strncmp(packet.filename,serFilename,strlen(serFilename))!=0){
        return 0;
    }


    memset(&packet, 0,sizeof(packet)); 
    strcpy(packet.servAddr,MyIP);
    strcpy(packet.filename,serFilename);
    //Set Random transmit port
    (*tempPort) = port+rand()%1000+33;
    packet.connectPort = (*tempPort);

    int SendBytes = sendto(socketfd, &packet,sizeof(packet), 0, 
        (struct sockaddr *)clientaddr,sizeof(struct sockaddr_in));
    if(SendBytes<0){perror("listenClient::sendto");exit(1);}


    printf("[Send Reply]\n\tFile:%s",packet.filename);
    printf("\n\tServer's IP address %s\n\tConnect Port is: %d\n",
        packet.servAddr,packet.connectPort);

    return 0;
}
int startMyftpServer( int tempPort, struct sockaddr_in *clientaddr, const char *filename ) {
    //In here new socket fd? and new bind?
    int tempSocketfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server_addrIpv4;
    memset(&server_addrIpv4,0,sizeof(struct sockaddr_in));
    server_addrIpv4.sin_family = AF_INET;
    server_addrIpv4.sin_port = htons(tempPort);
    printf("tempPort=%d",tempPort);
    server_addrIpv4.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(tempSocketfd, (struct sockaddr *)&server_addrIpv4,sizeof(struct sockaddr_in)) < 0) {
        perror("startMyftpServer::bind_error");exit(1);
    }
    int sock_opt = 1;
    if (setsockopt(tempSocketfd, SOL_SOCKET, SO_REUSEADDR, (void*)&sock_opt,sizeof(sock_opt) ) == -1){
        return -1;
    }
    //-------------Set Server Time out -----------
    struct timeval timeout;      
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    
    if (setsockopt (tempSocketfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(struct timeval)) < 0){
        perror("setsockopt SO_RCVTIMEO failed\n");exit(1);    
    }

    

//-------------inital FrqPacket------------
    struct myFtphdr *FrqPacket;
    int FileLen = strlen(filename)+1;//cause need '\0'
    int FrqSize =sizeof(short)+sizeof(unsigned short)+FileLen;
    FrqPacket = (struct myFtphdr *)malloc(FrqSize);
    memset(FrqPacket, 0,FrqSize); 
   
//-------------Server Get FRQ begin------------
//-------------Server recvfrom------------  
    int TimeoutNum=0;
    printf("\t[Wait Client FRQ]\n");
    for(;;){
        
        memset(FrqPacket, 0,FrqSize); 
        socklen_t clientAddrLen =sizeof(struct sockaddr_in);
       
        int RecBytes  = recvfrom(tempSocketfd, FrqPacket,FrqSize,MSG_WAITALL,(struct sockaddr *)clientaddr, &clientAddrLen);
       
        if(RecBytes<0){
            
            if(TimeoutNum>10){
                
                printf("[Wait Client FRQ Timeout >20sec]::Server exit\n");
                exit(1);
            }
            TimeoutNum++;
            //printf("mf_opcode=%d\n",FrqPacket->mf_opcode);
            printf("[Again Wait Client FRQ]\n");
           
            continue;
        }
        if(RecBytes==0){
           
            perror("startMyftpServer::recvfrom:RecBytes==0:Client Shotdown:\n");
            close(tempSocketfd);exit(1);
        }
        if(in_cksum((unsigned short *)FrqPacket,FrqSize)!=0){

            printf("Receive FRQ packet but checksum Error\n");
            continue;
        }
        printf("FRQ packet filename is %s\n",FrqPacket->mf_filename);
        break;
    }

//-------------FRQ end--------------

//-------------inital FilePacket------------
    struct myFtphdr *FilePacket;
    int FilePacketSize =sizeof(short)+sizeof(unsigned short)+sizeof(unsigned short)+MFMAXDATA;
    FilePacket = (struct myFtphdr *)malloc(FilePacketSize);
    int block = 1;
    int GetBlock0=0;
//-------------inital PreFilePacket------------
    struct myFtphdr *PreFilePacket;
    
//-------------inital AckPacket------------
    struct myFtphdr *AckPacket;
    int ACKsize =6;//sizeof(short)+sizeof(unsigned short)+sizeof(unsigned short)
    printf("ACKsize = %d",ACKsize);
    AckPacket = (struct myFtphdr *)malloc(ACKsize);

    FILE *fin = fopen(filename,"rb");

    printf("\n[file transmission start]\n");
    printf("\t\tSend file : <%s> to %s \n", FrqPacket->mf_filename, inet_ntoa(clientaddr->sin_addr));
    printf("\t\tclient_potr = %d\n",ntohs(clientaddr->sin_port));

    int ReadSize;

    
    struct myFtphdr *EndFilePacket;
//-------------Star Transmission File-----------
    for(;;){
        if(GetBlock0==1){break;}
        
        PreFilePacket = (struct myFtphdr *)malloc(FilePacketSize);
        memcpy(PreFilePacket,FilePacket,FilePacketSize);

        memset(FilePacket, 0,FilePacketSize); 
        

        ReadSize = fread(FilePacket->mf_data,1,MFMAXDATA,fin);

        //if data is 512*k , at end ReadSize = 0
        if(feof(fin) && ReadSize<MFMAXDATA){
            printf("Read End of File!!\n");
            printf("End ReadSize = %d\n",ReadSize);

            
            
            FilePacketSize = ReadSize+sizeof(short)+sizeof(unsigned short)+sizeof(unsigned short);

            EndFilePacket = (struct myFtphdr *)malloc(FilePacketSize);
            memcpy(EndFilePacket->mf_data,FilePacket->mf_data,ReadSize);
            free(FilePacket);
            FilePacket = EndFilePacket;

            FilePacket->mf_opcode=htons(DATA);
            FilePacket->mf_cksum=htons(0);
            FilePacket->mf_block = htons(block);
            FilePacket->mf_cksum=in_cksum((unsigned short *)FilePacket,FilePacketSize);
        }else{
            
            FilePacket->mf_opcode=htons(DATA);
            FilePacket->mf_cksum=htons(0);
            FilePacket->mf_block = htons(block);
            FilePacket->mf_cksum=in_cksum((unsigned short *)FilePacket,FilePacketSize);
        }
        //-------------Sending File-----------
        if((sendto(tempSocketfd,FilePacket,FilePacketSize ,0, (struct sockaddr *)clientaddr,sizeof(struct sockaddr_in)))<0){
            perror("sendto::DATA1::\n");exit(1);
        }


        //-------------Sending File may Error ,So need  resending-----------
        int TimeoutNum=0;
        for(;;){
            socklen_t clientAddrLen =sizeof(struct sockaddr_in);
            int RecBytes = recvfrom(tempSocketfd,AckPacket,ACKsize,0,(struct sockaddr*)clientaddr,&clientAddrLen);
            if(RecBytes<0){
                printf("[Send data again and wait ACK]TimeoutNum=%d\n",TimeoutNum);
                if(TimeoutNum>10){
                    printf("[File ACK drop continuous  Timeout >20sec]::Client exit\n");
                    exit(1);
                }
                TimeoutNum++;

                FilePacket->mf_opcode=htons(DATA);
                FilePacket->mf_cksum=htons(0);
                FilePacket->mf_block = htons(block);
                FilePacket->mf_cksum=in_cksum((unsigned short *)FilePacket,FilePacketSize);
                if((sendto(tempSocketfd,FilePacket,FilePacketSize ,0, (struct sockaddr *)clientaddr, sizeof(struct sockaddr_in)))<0){
                    perror("sendto::DATA1::\n");exit(1);
                }

                continue;
            }
            TimeoutNum=0;

            if(RecBytes==0){
                printf("[Client Shotdownown , End]\n");

                exit(1);
            }

            //-------------Receive previous delay FRQ , Ignore-----------
            if(ntohs(AckPacket->mf_opcode) == FRQ){
                
                continue;
            }

            //-------------Get ACK Error ,Send again-----------
            if(ntohs(AckPacket->mf_opcode) == ERROR){       
            	
                if(ntohs(AckPacket->mf_block) == block-1){
                	printf("[Get current Error Ack %d,Resend %d]\n",block-1,block);
                    FilePacket->mf_opcode=htons(DATA);
                    FilePacket->mf_cksum=htons(0);
                    FilePacket->mf_block = htons(block);
                    FilePacket->mf_cksum=in_cksum((unsigned short *)FilePacket,FilePacketSize);
                    if((sendto(tempSocketfd,FilePacket,FilePacketSize ,0, (struct sockaddr *)clientaddr, sizeof(struct sockaddr_in)))<0){
                        perror("sendto::DATA2::\n");exit(1);
                    }
                }else {
                	/*
                	printf("[Get previous delay Error Ack,mf_block=%d ,block=%d ,Resend=%d]\n",ntohs(AckPacket->mf_block),block,ntohs(PreFilePacket->mf_block));
                    if((sendto(tempSocketfd,PreFilePacket,FilePacketSize ,0, (struct sockaddr *)clientaddr, sizeof(struct sockaddr_in)))<0){
                        perror("sendto::DATA3::\n");exit(1);
                    }*/
                        //Ignore
                }
            	continue;

            }

            //-------------Check sum error,Send again-----------
            if(in_cksum((unsigned short *)AckPacket,ACKsize)!=0){
                printf("Check sum error\n");
               
                FilePacket->mf_opcode=htons(DATA);
                    FilePacket->mf_cksum=htons(0);
                    FilePacket->mf_block = htons(block);
                    FilePacket->mf_cksum=in_cksum((unsigned short *)FilePacket,FilePacketSize);
                    if((sendto(tempSocketfd,FilePacket,FilePacketSize ,0, (struct sockaddr *)clientaddr, sizeof(struct sockaddr_in)))<0){
                        perror("sendto::DATA3::\n");exit(1);
                }
                continue;
            }

            //-------------Check sum ACK , If all ok block++-----------
            if(ntohs(AckPacket->mf_opcode) == ACK){
                if(ntohs(AckPacket->mf_block) == 0){
                    
                    printf("Receive block 0 \n[End of Transmission]\n[wait new client]");
                    GetBlock0=1;
                    fclose(fin);
                    break;
                }

                if (ntohs(AckPacket->mf_block) != block){
                	printf("[Old Ack %d Ignore,and Wait]\n",AckPacket->mf_block);
                    //timeout Old Ack,Ignore
                    continue;
                }
                if(ntohs(AckPacket->mf_block) == block){
                    //Successful AckPacket
                    block++;
                    if(block == 65000){
                        //Client restar is 1,not 0, So Server restaris 2 not 1
                        block = 2;
                    }
                    break;
                }
            }

        }//Inner Loop


    }//Loop


    return 0;
}

int startMyftpClient(struct sockaddr_in *servaddr, const char *filename ) {
    int socketfd;
    if((socketfd= socket(AF_INET,SOCK_DGRAM,0))<0){
        perror("startMyftpClient::socket");
        exit(1);
    }
    //-------------Set Client Time out -----------
    struct timeval timeout;      
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (setsockopt (socketfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(struct timeval)) < 0){
        perror("startMyftpClient::setsockopt SO_RCVTIMEO failed\n");exit(1);    
    }
    
    //-------------inital FrqPacket------------
    struct myFtphdr *FrqPacket;
    int FileLen = strlen(filename)+1;
    int FrqSize =sizeof(short)+sizeof(unsigned short)+FileLen;
    printf("%lu %lu\n",sizeof(short),sizeof(unsigned short));
    FrqPacket = (struct myFtphdr *)malloc(FrqSize);
    memset(FrqPacket, 0,FrqSize);    
   
    //-------------Sending FrqPacket------------
    FrqPacket->mf_opcode = htons(FRQ);
    FrqPacket->mf_cksum=htons(0);
    strcpy(FrqPacket->mf_filename,filename);
    FrqPacket->mf_cksum=in_cksum((unsigned short *)FrqPacket,FrqSize);
    
    printf("startMyftpClient servaddr %d %s\n",ntohs(servaddr->sin_port),inet_ntoa(servaddr->sin_addr));
    socklen_t len = sizeof(struct sockaddr_in);
    int t=-1;
    if( (t=sendto(socketfd,FrqPacket,FrqSize,0,(struct sockaddr *)servaddr,len)) == -1){
        exit(1);
    }
    //-------------inital FilePacket------------
    struct myFtphdr *FilePacket;
    int MaxFilePacketSize =sizeof(short)+sizeof(unsigned short)+sizeof(unsigned short)+MFMAXDATA;
    FilePacket = (struct myFtphdr *)malloc(MaxFilePacketSize);
    
    //-------------inital AckPacket------------
    struct myFtphdr *AckPacket;
    int AckSize =6;
    AckPacket = (struct myFtphdr *)malloc(AckSize);

    //-------------inital Write File------------
    FILE *fin;
    char RecvFile[FNAMELEN];
    memset(RecvFile, 0,sizeof(RecvFile)); 
    sprintf(RecvFile,"client_%s",filename);
    fin=fopen(RecvFile,"wb");

    //-------------Transmission recvfrom & Ack------------
    int block = 0;
    int TimeoutFirstNum=0,TimeoutNum=0;
    for(;;){
        
        socklen_t recvlen=sizeof(struct sockaddr_in);
        memset(FilePacket, 0,MaxFilePacketSize); 
        int RecBytes=recvfrom(socketfd,FilePacket,MaxFilePacketSize,0,(struct sockaddr*)servaddr,&recvlen);
        if(RecBytes<0){
          
            if(block == 0){
                //block not update , sercer may not get frq
                if(TimeoutFirstNum>10){
                    printf("[No First Data Timeout>20sec] exit\n");
                    return 0;
                }
                printf("[No First Data] send FRQ packet again\n");
                TimeoutFirstNum++;
                if( (t=sendto(socketfd,FrqPacket,FrqSize,0,(struct sockaddr *)servaddr,len)) == -1){
                    exit(1);
                }
                //printf("sendtoBytes=%d  mf_opcode=%d %d !%s!\n",t,ntohs(FrqPacket->mf_opcode),ntohs(FrqPacket->mf_cksum),FrqPacket->mf_filename);
                continue;
            }else{
                //block had update , sercer had get frq
                if(TimeoutNum>10){
                    printf("[No Server Data Timeout>20sec] exit\n");
                    return 0;
                }
                TimeoutNum++;
                printf("[Resend ERROR Ack Block %d] Let Server to resend data TimeoutNum=%d\n",block,TimeoutNum);
                AckPacket->mf_opcode=htons(ERROR);
                AckPacket->mf_cksum=htons(0);
                AckPacket->mf_block = htons(block);
                AckPacket->mf_cksum=in_cksum((unsigned short *)AckPacket,AckSize);
                if((sendto(socketfd, AckPacket,AckSize ,0, (struct sockaddr *)servaddr, sizeof(struct sockaddr_in)))<0){
                    perror("sendto:ACK1:");exit(1);
                }
                continue;
            }          
        }//End if(RecBytes<0)

        TimeoutFirstNum=0;
        TimeoutNum=0;
        if(RecBytes==0){
            //server shotdown
            printf("[Server Shotdown]\n");
            fclose(fin);
            close(socketfd);
            return 0;
        }
        
        //-------Checksum Error bit,send again----------
        if(in_cksum((unsigned short *)FilePacket,MaxFilePacketSize)!=0){
           
            printf("[Received data checksum error]\n");

            AckPacket->mf_opcode=htons(ERROR);
            AckPacket->mf_cksum=htons(0);
            AckPacket->mf_block = htons(block);
            AckPacket->mf_cksum=in_cksum((unsigned short *)AckPacket,AckSize);
            if((sendto(socketfd, AckPacket,AckSize ,0, (struct sockaddr *)servaddr, sizeof(struct sockaddr_in)))<0){
                perror("sendto:ACK3:");exit(1);
            }
            continue;
        }
        //-------Server dose not receive previous Ack----------
        if(ntohs(FilePacket->mf_opcode) == DATA && ntohs(FilePacket->mf_block) != block+1){
            printf("[Server dose not receive previous Ack;Get FilePacket->mf_block=%d]\n",ntohs(FilePacket->mf_block));
            int previous = ntohs(FilePacket->mf_block);
            AckPacket->mf_opcode=htons(ACK);
            AckPacket->mf_cksum=htons(0);
            AckPacket->mf_block = htons(previous);
            AckPacket->mf_cksum=in_cksum((unsigned short *)AckPacket,AckSize);
            if((sendto(socketfd, AckPacket,AckSize ,0, (struct sockaddr *)servaddr, sizeof(struct sockaddr_in)))<0){
                perror("sendto:ACK2:");exit(1);
            }
            continue;
        }
        //-------Server receive previous Ack----------
        if(ntohs(FilePacket->mf_opcode) == DATA && ntohs(FilePacket->mf_block) == block+1){
            
            
            int WriteBytes = RecBytes-6;
            fwrite(FilePacket->mf_data,1,WriteBytes,fin);
            
            if(WriteBytes<MFMAXDATA){
            //-------Successful Finish, Send ACK----------
                printf("\n[Transmission finish!!]\n");
                block = 0;

                AckPacket->mf_opcode=htons(ACK);
                AckPacket->mf_cksum=htons(0);
                AckPacket->mf_block = htons(block);
                AckPacket->mf_cksum=in_cksum((unsigned short *)AckPacket,AckSize);

                if((sendto(socketfd, AckPacket,AckSize ,0, (struct sockaddr *)servaddr, sizeof(struct sockaddr_in)))<0){
                    perror("sendto:ACK2:");exit(1);
                }

                fclose(fin);
                break;
            }
            else{
            //-------Successful , Send ACK----------
         
            block = ntohs(FilePacket->mf_block);

            AckPacket->mf_opcode=htons(ACK);
            AckPacket->mf_cksum=htons(0);
            AckPacket->mf_block = htons(block);
            AckPacket->mf_cksum=in_cksum((unsigned short *)AckPacket,AckSize);
            
            if((sendto(socketfd, AckPacket,AckSize ,0, (struct sockaddr *)servaddr, sizeof(struct sockaddr_in)))<0){
                perror("sendto:ACK2:");exit(1);
            }

            	
                if(block == 64999){
                    //Cause block restar should not at 0 , use 1
                    block = 1;
                }
            }
        }

    }


    close(socketfd);
    return 0;
}
unsigned short in_cksum(unsigned short *addr, int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short *w = addr;
    unsigned short answer = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1) {
        *(unsigned char *) (&answer) = *(unsigned char *) w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    answer = ~sum;
    return (answer);
}