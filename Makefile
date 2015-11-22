
all: tfrc-client

tfrc-client: tfrc-client.o tfrc.o clientresources.o
	gcc -Wall -o tfrc-client tfrc-client.o tfrc.o clientresources.o 

tfrc-client.o: tfrc_client.c 
	gcc -Wall -o tfrc-client.o -c tfrc_client.c

clientresources.o: clientresources.c
	gcc -Wall -o clientresources.o -c clientresources.c

tfrc.o: tfrc.c
	gcc -Wall -o tfrc.o -c tfrc.c

clean:
	rm -f *.o *~ server tfrc-client core

