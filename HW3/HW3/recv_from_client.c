#include "header.h"


void *recv_from_client(void *info) {


	Member* client = (Member*)info;
	char SendMessage[SHORTMESS];
	char RecvMessage[SHORTMESS];

	/**Client Input NickName star**/
	memset(SendMessage, '\0',SHORTMESS );
	strcpy(SendMessage,"[Welcom] please input Your NickName:\0");
	if( send(client->fd,SendMessage, SHORTMESS, 0) < 0) {
		DIE("send_error0");
	}
	memset(RecvMessage, '\0',SHORTMESS );
	if( recv(client->fd,RecvMessage, SHORTMESS, 0) < 0) {
		DIE("recv_error0");
	}

	memset(client->name, '\0',SHORTMESS );
	strcpy(client->name,RecvMessage);
	printf("%s\n", client->name);
	/**Client Input NickName end**/


	/**Client Select Group star**/

	memset(SendMessage, '\0',SHORTMESS );
	strcpy(SendMessage,"Member[Group] \n\0");

	if(MemberListSize()==0){
		strcat( SendMessage,"No Group");
	}
	else{

		int i=0,size=MemberListSize();
pthread_mutex_lock(&member_mutex);
		Member* temp = MemberListFirst;
		
		for(i=0; i<size; i++){
			
				strcat( SendMessage, temp->name  );
				strcat(SendMessage,"[\0");
				strcat( SendMessage, temp->group  );
				strcat(SendMessage,"],\0");

				if(temp->next==NULL)break;
				
				temp = temp->next;
		}

pthread_mutex_unlock(&member_mutex);
	}

	strcat( SendMessage," Please Select Group:");
	if( send(client->fd,SendMessage, SHORTMESS, 0) < 0) {
		DIE("send_error1");
	}

	memset(RecvMessage, '\0',SHORTMESS );
	if( recv(client->fd,RecvMessage, SHORTMESS, 0) < 0) {
		DIE("recv_error1");
	}

	memset(client->group, '\0',SHORTMESS );
	strcpy(client->group,RecvMessage);
	printf("%s\n", client->group);
	/**Client Select Group end**/


	/**Broadcast to All Member someone  name login group**/
	int i;
	Member* temp = MemberListFirst;
	memset(SendMessage, '\0',SHORTMESS );
	strcpy(SendMessage,"!!Server Note Client ");
	strcat( SendMessage, client->name  );
	strcat(SendMessage," Login ");
	strcat( SendMessage, client->group  );
	strcat(SendMessage,"\n\0");

	pthread_mutex_lock(&member_mutex);
	for(i=0; i<MemberListSize(); i++){
			
			if( send(temp->fd,SendMessage, SHORTMESS, 0) < 0) {
				DIE("send_error1");
			}
			if(temp->next==NULL)break;
				
			temp = temp->next;

		}
	pthread_mutex_unlock(&member_mutex);

	/**this client push to List**/
	pthread_mutex_lock(&member_mutex);
	MemberLinkToList(client);
	pthread_mutex_unlock(&member_mutex);



	memset(SendMessage, '\0',SHORTMESS );
	strcpy(SendMessage,"Usage ::\n\t<Message>\n\t/W <NickName Or Group> <Message>\n\tBye\n\0");
	if( send(client->fd,SendMessage, SHORTMESS, 0) < 0) {
				DIE("send_error1");
	}

	memset(RecvMessage, '\0',SHORTMESS );
	while(recv(client->fd,RecvMessage, SHORTMESS, 0) != -1){

		if (  strncmp(RecvMessage,"Bye",3)==0 && strlen(RecvMessage)<=5) {
			break;
		}
		//printf("!!%s!!\n",RecvMessage );


		Message* test = (Message*) malloc(sizeof(Message));
		strcpy( test->from , client->name);
		strcpy( test->message , RecvMessage);
		test->thread = client->thread;
		
		strcpy(test->SenderGroup,client->group);

		pthread_mutex_lock(&message_mutex);
		MessageLinkToList(test);
		pthread_mutex_unlock(&message_mutex);


		

		memset(RecvMessage, '\0',SHORTMESS );
	}
	
	printf("End of while (recv)");




	/**Broadcast to All Member someone  Exit**/
	memset(SendMessage, '\0',SHORTMESS );
	strcpy(SendMessage,"!!Server Note Client ");
	strcat( SendMessage, client->name  );
	strcat(SendMessage," Exit");
	strcat(SendMessage,"\n\0");
	temp = MemberListFirst;
	while(temp != NULL){
	

			if( send(temp->fd,SendMessage, SHORTMESS, 0) < 0) {
				DIE("send_error1");
			}
			if(temp->next==NULL)break;
				
			temp = temp->next;
	}
	close(client->fd);

/**Use pthread id to update List**/
	pthread_mutex_lock(&member_mutex);
	MemberDislinkToList(client);
	free(client);
	pthread_mutex_unlock(&member_mutex);

	return NULL;
}

void MemberLinkToList(Member* client) {
	
	if(MemberListFirst == NULL)
	{
		//printf("MemberListFirst == NULL ,and it will make new list because new one come\n");
		MemberListFirst = client;
		
	}
	else
	{
		//printf("MemberListFirst == %x ,and it will change list because new one come\n",MemberListFirst);
	

		/**Insert at First**/
		MemberListFirst->prev=client;
		client->next=MemberListFirst;
		MemberListFirst=client;
	}
	return ;
}
void MemberDislinkToList(Member* client){

	
	if(client->prev==NULL)
	{
		MemberListFirst=client->next;


		/**IN Here when MemberListFirst==NULL => MemberListFirst==client && just one in list**/

		if(MemberListFirst!=NULL)
			MemberListFirst->prev=NULL;
	}
	else if(client->prev!=NULL && client->next==NULL)
	{
		client->prev->next=client->next;
	}
	else if(client->prev!=NULL && client->next!=NULL)
	{
		client->prev->next=client->next;
		client->next->prev=client->prev ;
	}
	else
	{
		DIE("1) Take the Member out of the circular list");
	}
	return ;


}

int  MemberListSize(){

	Member* temp = MemberListFirst;
	int i=0;
	
	while(temp!=NULL){
		i++;

		if(temp->next==NULL)
			break;
			
		
		temp=temp->next;
	}


	return i;
}


