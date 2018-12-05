#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <stdbool.h>

#define MAXBUFSIZE 2048

int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	char buffer[MAXBUFSIZE];
	bool flag = false;
	char userInput[MAXBUFSIZE];
	char fileName[MAXBUFSIZE];
	struct sockaddr_in remote_addr;              //"Internet socket address structure"

	char get[] = "get";
	char put[] = "put";
	char delete[] = "delete";
	char ls[] = "ls";
	char exitServer[] = "exit";
	char over[] = "Over";
	int fd;
	char* splitInput;

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	/******************
	  Here we populate a sockaddr_in struct with
	  information regarding where we'd like to send our packet
	  i.e the Server.
	 ******************/
	bzero(&remote_addr,sizeof(remote_addr));               //zero the struct
	remote_addr.sin_family = AF_INET;                 //address family
	remote_addr.sin_port = htons(10001);      //sets port to network byte order
	remote_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //sets remote IP address

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		printf("unable to create socket");
    return -1;
	}

  if(connect(sock, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0) {
    printf("Error on connect.\n");
    return -1;
  }


	/******************
	  sendto() sends immediately.
	  it will report an error if the message fails to leave the computer
	  however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	 ******************/
	struct sockaddr_in server_addr;
	unsigned int addr_length= sizeof(server_addr);

	while(1) {
		printf("\n\nHere is a list of commands: [get, put, ls, delete, exit]\n");
		printf("What would you like to do?\n");
		fgets(userInput, MAXBUFSIZE, stdin);
		splitInput = strtok(userInput, " ");
		printf("EXECUTING: %s\n", splitInput);
    write(sock, get, strlen(get));
    nbytes = read(sock, buffer, MAXBUFSIZE);
    printf("BUFFER: %s\n", buffer);

	}
	close(sock);
}
