all:Task_myping


CC = gcc
Task_myping:main.o pcap.o fill_packet.o
	$(CC) -o myping main.o pcap.o fill_packet.o -lpcap

%.o:%.c
	$(CC) -c  $<
#$<：	代表第一個 dependencies 的值。

clean :
	rm -f *.o myping
