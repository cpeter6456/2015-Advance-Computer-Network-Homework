#include "header.h"


void MessageLinkToList(Message* client) {
	
	if(MessageListFirst == NULL)
	{
		//printf("MessageListFirst == NULL ,and it will make new list because new one come\n");
		MessageListFirst = client;
		
	}
	else
	{
		//printf("MessageListFirst == %x ,and it will change list because new one come\n",MessageListFirst);
	

		/**Insert at First**/
		MessageListFirst->prev=client;
		client->next=MessageListFirst;
		MessageListFirst=client;
	}
	return ;
}
void MessageDislinkToList(Message* client){

	
	if(client->prev==NULL)
	{
		MessageListFirst=client->next;


		/**IN Here when MessageListFirst==NULL => MessageListFirst==client && just one in list**/

		if(MessageListFirst!=NULL)
			MessageListFirst->prev=NULL;
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
		DIE("1) Take the Message out of the circular list");
	}
	

	return ;
}

int  MessageListSize(){

	Message* temp = MessageListFirst;
	int i=0;
	
	while(temp!=NULL){
		i++;

		if(temp->next==NULL)
			break;
			
		
		temp=temp->next;
	}


	return i;
}


/**Send To Client (may Group or Name Or Local Group)**/



void *send_to_client(void *info) {


	while(1){

		pthread_mutex_lock(&message_mutex);
		char SendMessage[SHORTMESS];

		int k;
		Message* tempM = MessageListFirst;
		for(k=0; k<MessageListSize(); k++){
			//printf("k=%d\n",k );
			memset(SendMessage, '\0',SHORTMESS );

			strcpy(SendMessage,tempM->message);
			printf("Origin SendMessage  = %s\n",SendMessage);
			int type=-1;/**0 is Local group ,1 is \W have send to same name or group**/

		

			char tempBuf[SHORTMESS];
			memset(tempBuf, '\0',SHORTMESS );
			strcpy(tempBuf,SendMessage);

			char delim[10] = " ";
			char * pch;
			pch = strtok(tempBuf,delim);


			char NameOrGroup[SHORTMESS];
			memset(NameOrGroup, '\0',SHORTMESS );


			if( strncmp(SendMessage,"/W ",3) != 0){
			/** Local group , NOt Change SendMessage**/
			/**Not Use strncmp(pch,"\W",2) avoid \Wdasdasd message**/
				type=0;
			}else{
				/**Change SendMessage**/

				pch = strtok(NULL,delim);
				strcpy(NameOrGroup,pch);

				pch+=strlen((pch))+1;
				memset(SendMessage, '\0',SHORTMESS );
				strcpy(SendMessage,pch);

				type=1;
			}

			char NameSendMessage[SHORTMESS];
			memset(NameSendMessage, '\0',SHORTMESS );
			strcpy(NameSendMessage,tempM->from);
			strcat(NameSendMessage," : ");
			strcat(NameSendMessage,SendMessage);
			strcat(NameSendMessage,"\n\0");
			//printf("NameSendMessage %s\n",NameSendMessage);
			//printf("%s\n","switch" );
			switch(type){
				case 0:
				/**	Local group	**/

				pthread_mutex_lock(&member_mutex);
							Member*	temp = MemberListFirst;
							int j=0;
							for(j=0;j<MemberListSize(); j++){

								if(strcmp(tempM->SenderGroup,temp->group)==0){
									if( send(temp->fd,NameSendMessage, SHORTMESS, 0) < 0) {
										DIE("case 0::send_to_client::send_error");
									}
								}
								if(temp->next==NULL)break;

								temp=temp->next;
							}

				pthread_mutex_unlock(&member_mutex);
					


					break;
				case 1:

				pthread_mutex_lock(&member_mutex);
					int i=0;
					Member*	tempMem = MemberListFirst;
					for(i=0; i<MemberListSize(); i++){

						if( strcmp(tempMem->group,NameOrGroup)==0 || strcmp(tempMem->name,NameOrGroup)==0){
							if( send(tempMem->fd,NameSendMessage, SHORTMESS, 0) < 0) {
										DIE("case 1::send_to_client::send_error");
							}
						}
						if(tempMem->next==NULL)break;

						tempMem=tempMem->next;

					}

				pthread_mutex_unlock(&member_mutex);
					break;
				
				default:
          		printf("%s input error",tempM->from);
			}

		}

		
		
		while(MessageListSize()>0){
			Message* tempM = MessageListFirst;
			MessageDislinkToList(tempM);
			free(tempM);
		}
		
		pthread_mutex_unlock(&message_mutex);

	}
	printf("%s\n","End send_to_client" );



	return NULL;
}

