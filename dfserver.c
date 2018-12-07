#include <sys/types.h>
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
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAXBUFSIZE 2048
int listening = 1;

void INThandler(int sig);
bool authHandler(char* credentials);

int main (int argc, char * argv[] )
{

	int sock, clientSock, pid;                           //This will be our socket
	struct sockaddr_in server_addr, client_addr;     //"Internet socket address structure"
	int nbytes;                        //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	int fd;
	int client_length = sizeof(client_addr);
  char authTrue[] = "VALID";
  char authFalse[] = "INVALID";

	DIR* dir;
	struct dirent* in_file;

	if (argc != 3)
	{
		printf ("USAGE: <server #> <port>\n");
		return -1;
	}

	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	bzero(&server_addr,sizeof(server_addr));                    //zero the struct
	server_addr.sin_family = AF_INET;                   //address family
	server_addr.sin_port = htons(atoi(argv[2]));        //htons() sets the port # to network byte order
	server_addr.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		printf("unable to create socket");
    return -1;
	}

  int enable = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		printf("Error on socket options.\n");
		return -1;
	}

	if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		printf("unable to bind socket\n");
    return -1;
	}

  if(listen(sock, 5) < 0) {
    printf("unable to listen on port\n");
    return -1;
  }
  bzero(&client_addr, client_length);

  signal(SIGINT, INThandler);
  while(listening) {
    // Accept Connection
    clientSock = accept(sock, (struct sockaddr *) &client_addr, (socklen_t *) &client_length);
    if(clientSock < 0) {
      perror("ERROR on accept.\n");
      exit(-1);
    }

    pid = fork();
    if(pid < 0) {
      perror("ERROR on fork.\n");
      exit(-1);
    }

    if(pid == 0) {
      close(sock);
 	 	  bzero(&buffer,sizeof(buffer));
      nbytes = read(clientSock, buffer, MAXBUFSIZE);
      if(nbytes < 0) {
 	       perror("ERROR reading from socket");
 	       exit(1);
 	    }

      // Auth
      if(!authHandler(buffer)) {
        write(clientSock, authFalse, strlen(authFalse));
        close(clientSock);
        exit(0);
      }
      else {
        write(clientSock, authTrue, strlen(authTrue));
      }

			while(1) {
				bzero(&buffer,sizeof(buffer));
				printf("Recieving bytes...\n");
				nbytes = read(clientSock, buffer, MAXBUFSIZE);
				printf("Recieved: %s\n", buffer);

				if(!strcmp(buffer, "get")) {
				  printf("EXECUTING: %s\n", buffer);
		    }
		    else if(!strcmp(buffer, "put")) {
				  printf("EXECUTING: %s\n", buffer);
					bzero(&buffer,sizeof(buffer));
					nbytes = read(clientSock, buffer, MAXBUFSIZE);
					printf("FileName: %s\n", buffer);
					bzero(&buffer,sizeof(buffer));
					nbytes = read(clientSock, buffer, MAXBUFSIZE);
					printf("File Contents: %s\n", buffer);

		    }
		    else if(!strcmp(buffer, "list")) {
		      printf("EXECUTING: %s\n", buffer);
		    }
				else if(!strcmp(buffer, "exit")) {
		      printf("EXECUTING: %s\n", buffer);
					break;
				}
		    else {
		      printf("Command unknown.\n");
		    }
			}

      close(clientSock);
      exit(0);
    }

    // Parent Process
    close(clientSock);

  }
}

void INThandler(int sig)
{
	signal(sig, SIG_IGN);
	listening = 0;
	exit(0);
}

bool authHandler(char* credentials)
{

  FILE* confFD;
  char fileBuffer[MAXBUFSIZE];

  confFD = fopen("dfs.conf", "r");
  if(confFD == NULL) {
    printf("Error opening file.\n");
    exit(0);
  }

  while(fgets(fileBuffer, sizeof(fileBuffer), confFD) != NULL) {
    fileBuffer[strlen(fileBuffer) - 1] = '\0';
    if(!strcmp(fileBuffer, credentials)) {
      return true;
    }
  }
  return false;

}
