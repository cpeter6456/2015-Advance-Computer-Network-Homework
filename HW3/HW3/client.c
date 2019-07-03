#include "header.h"

pthread_attr_t attr;		/* set to make ALL threads detached		*/
void *recv_from_server(void *info);
char RecvMessage[SHORTMESS],SendMessage[SHORTMESS];
int SockDescriptor ;
int main(int argc, char **argv){

	struct sockaddr_in server_addrIpv4;
	server_addrIpv4.sin_family = AF_INET;

	/**B013040033 is 51033**/
	

	server_addrIpv4.sin_addr.s_addr = inet_addr(argv[1]);
	int port = atoi(argv[2]);
	server_addrIpv4.sin_port = htons(port);

	SockDescriptor = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(SockDescriptor, (struct sockaddr *)&server_addrIpv4, sizeof(server_addrIpv4)) < 0) {
		perror("connect_error");
	}


	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);				/**set detach **/
/**set PTHREAD_CREATE_DETACHED  这样在线程结束的时候资源(线程的描述信息和stack)才能被释放.**/


	pthread_t recv_from_server_pthread;
	pthread_create(&recv_from_server_pthread	, &attr, &recv_from_server, 	NULL);    /** 執行緒 recv_from_server_pthread**/

	
	while(1){
		memset(SendMessage, '\0',SHORTMESS );
		

		fgets(SendMessage, SHORTMESS , stdin);//abc\n\0 len is 4
		SendMessage[strlen(SendMessage)-1]='\0';
		//printf("SendMessage = %s\n", SendMessage);

		
		if( send(SockDescriptor,SendMessage, SHORTMESS, 0) < 0) {
			DIE("send_error");
		}




		if(strncmp(SendMessage,"Bye",3)==0 && strlen(SendMessage)<=5 ){break;}

	


	}
	printf("GoodBye\n" );
	close(SockDescriptor);
	
	return 0;
}
void *recv_from_server(void *info){

	while(recv(SockDescriptor,RecvMessage, SHORTMESS, 0) != -1){

	
		printf("%s", RecvMessage );
		fflush(stdout);
		memset(RecvMessage, '\0',SHORTMESS );
	}

	return NULL;
}
