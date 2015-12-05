
all: tfrc-client tfrc-server

tfrc-server: tfrc-server.o logQueue.o lossLList.o tfrc.o
	gcc -Wall tfrc-server.o tfrc.o logQueue.o lossLList.o -o tfrc-server -lm

tfrc-server.o: tfrc-server.c
	gcc -Wall -o tfrc-server.o -c tfrc-server.c
logQueue.o: logQueue.c
	gcc -Wall -o logQueue.o -c logQueue.c
lossLList.o: lossLList.c
	gcc -Wall -o lossLList.o -c lossLList.c
tfrc-client: tfrc-client.o tfrc.o clientresources.o
	gcc -ggdb -Wall -o tfrc-client tfrc-client.o tfrc.o clientresources.o -lpthread -lm

tfrc-client.o: tfrc_client.c 
	gcc -ggdb -Wall -o tfrc-client.o -c tfrc_client.c -lpthread -lm

clientresources.o: clientresources.c
	gcc -ggdb -Wall -o clientresources.o -c clientresources.c -lm

tfrc.o: tfrc.c
	gcc -ggdb -Wall -o tfrc.o -c tfrc.c -lm

clean:
	rm -f *.o *~ tfrc-server tfrc-client core

