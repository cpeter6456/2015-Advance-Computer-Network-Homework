all:Task_arp


CC = gcc
Task_arp:arp.o main.o 
	$(CC) -o arp arp.o main.o -lnsl -lpthread



%.o:%.c
	$(CC) -c  $<

clean :
	rm -f *.o arp
