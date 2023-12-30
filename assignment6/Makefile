all: client csapp 
	gcc client.o csapp.o -lpthread -o client

client: 
	gcc -c -Wall -O2 client.c

csapp:
	gcc -c -Wall -O2 csapp.c

clean:
	rm -f client csapp.o client.o




