all:Task_chat_server Task_chat_client


CC = gcc
Task_chat_server:server.o recv_from_client.o send_to_client.o
	$(CC) -o server server.o recv_from_client.o send_to_client.o -lnsl -lpthread
Task_chat_client:client.o 
	$(CC) -o client client.o -lnsl -lpthread



%.o:%.c
	$(CC) -c -Wall $<

clean :
	rm -f *.o client server 
