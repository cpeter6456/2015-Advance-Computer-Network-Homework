
#ifndef CHAT_HEADER
#define CHAT_HEADER

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
//#include <list.h>
//#include <vector>

//using namespace std;

#define DIE(x)   perror(x),exit(errno)

#define SHORTMESS 512
#define MAXDEST 64
typedef struct message
{
	pthread_t thread;	
	char from[SHORTMESS];
    char message[SHORTMESS];
    char SenderGroup[SHORTMESS];
    struct message * prev;		/* Doubly linked list					*/
  	struct message * next;		/* Doubly linked list					*/
}Message;

typedef struct member {
	int fd;							/* Connect to it client Socket connection					*/
	pthread_t thread;				/* For control	(Can know which client)	*/
	char name[SHORTMESS];
	char group[SHORTMESS];
	struct member * prev;		/* Doubly linked list					*/
  	struct member * next;		/* Doubly linked list					*/


} Member;

void MemberLinkToList(Member*);
void MemberDislinkToList(Member*);
int  MemberListSize();



void MessageLinkToList(Message*);
void MessageDisLinkToList(Member*);
int  MessageListSize();


//extern vector <Member*> Member_vector;
//extern vector <Message*> Message_vector;

 
extern Member* 	MemberListFirst;   

extern Message* MessageListFirst;   

extern pthread_mutex_t member_mutex;   
extern pthread_mutex_t message_mutex;  



#endif