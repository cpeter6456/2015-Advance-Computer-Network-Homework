all:Task_OTHServer Task_OTHClient


CC = gcc
Task_OTHServer:myftpserver.o myftp.o 
	$(CC) -o Server myftpserver.o myftp.o  -lnsl -lpthread

Task_OTHClient:myftpclient.o myftp.o 
	$(CC) -o Client myftpclient.o myftp.o  -lnsl -lpthread

%.o:%.c
	$(CC) -c  $<

clean :
	rm -f *.o arp Server Client
