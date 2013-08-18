Server: Server.o
	gcc -o Server Server.o -lpthread
Client: Client.o	
	gcc -o Client Client.o -lpthread
Server.o: Serv.c Protocol.h
	gcc -c -o Server.o Serv.c -lpthread
Client.o: Cli.c Protocol.h 
	gcc -c  -o Client.o Cli.c 
clean:
	rm -f Server  Client *.o
