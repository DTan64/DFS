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
	char sendBuffer[MAXBUFSIZE];             //a buffer to store our received message
	char filePath[MAXBUFSIZE];             //a buffer to store our received message
	char directoryPath[MAXBUFSIZE];
	FILE* fd;
	int fp;
	int readBytes;
	int client_length = sizeof(client_addr);
  char authTrue[] = "VALID";
  char authFalse[] = "INVALID";
	char* splitInput;
	int i;
	int fileSize;
	int len;
	char fileDeleteBuffer[MAXBUFSIZE];

	DIR* dir;
	struct dirent* in_file;
	struct stat st;

	char* fileName;

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
				splitInput = strtok(buffer, " ");
 	 	  	bzero(&filePath, sizeof(filePath));
				strcat(filePath, "./");
				strcat(filePath, argv[1]);
				strcat(filePath, "/");
				strcat(filePath, splitInput);
				strcpy(directoryPath, filePath);
				dir = opendir(filePath);
				if(ENOENT == errno) {
					printf("Directory doesn't exist\n");
 	 	  		bzero(&buffer, sizeof(buffer));
					strcpy(buffer, filePath);
 	 	  		bzero(&filePath, sizeof(filePath));
					strcat(filePath, "mkdir ");
					strcat(filePath, buffer);
					system(filePath);
				}
				else {
					closedir(dir);
				}
      }

			while(1) {
				bzero(&buffer,sizeof(buffer));
				printf("Recieving bytes...\n");
				nbytes = read(clientSock, buffer, MAXBUFSIZE);
				if(nbytes <= 0) {
					break;
				}
				printf("Recieved: %s\n", buffer);


				if(!strcmp(buffer, "get")) {
				  printf("EXECUTING: %s\n", buffer);

					bzero(&buffer,sizeof(buffer));
					nbytes = read(clientSock, buffer, MAXBUFSIZE);
					printf("buffer: %s\n", buffer); // fileName

					// send back which numbers you have and files. and client handles rest.
					// for ls maybe just check if a connection is there?

					printf("directoryPath%s\n", directoryPath);
					dir = opendir(directoryPath);
					if(dir == NULL) {
						printf("Error opening directory\n");
						return -1;
					}

					// loop over directory
					while((in_file = readdir(dir))) {
						if(!strcmp(in_file->d_name, ".") || !strcmp(in_file->d_name, ".."))
							continue;
						printf("fileName %s\n", in_file->d_name);
						fileName = (char *) malloc(255);
						strcpy(fileName, in_file->d_name);
						fileName++;
						fileName[strlen(fileName) - 1] = '\0';
						fileName[strlen(fileName) - 1] = '\0';
						printf("fileName pointer: %s\n", fileName);
						if(!strcmp(buffer, fileName)) {
		  			  bzero(&sendBuffer, sizeof(sendBuffer));
		  			  bzero(&filePath, sizeof(filePath));
							strcat(sendBuffer, &in_file->d_name[strlen(in_file->d_name) - 1]);
							printf("sendBuffer: %s\n", sendBuffer);
							write(clientSock, &sendBuffer, MAXBUFSIZE);
							strcat(filePath, directoryPath);
							strcat(filePath, "/");
							strcat(filePath, in_file->d_name);
							printf("filePath: %s\n", filePath);

							fp = open(filePath, O_RDONLY);
		  			  if(fp < 0) {
		  			 	 	printf("Error opening file.\n");
		 				  	exit(-1);
		  			  }

		  			  bzero(&sendBuffer, sizeof(sendBuffer));
		  			  while(1) {
								printf("inside while loop %s\n", filePath);
		  			 	  readBytes = read(fp, sendBuffer, sizeof(sendBuffer));
		  			 	  if(readBytes == 0) {
									printf("didn't read\n");
									break;
		  			 	  }
								printf("sending: %s\n", sendBuffer);
		  			 	  len = write(clientSock, sendBuffer, MAXBUFSIZE);
								if(len < 0) {
									printf("ERROR ON SEND\n");
								}
		  			  	bzero(&sendBuffer, sizeof(sendBuffer));
		  			  }
			 	 		 	close(fp);
						}
						else {
							printf("hit else\n");
						}
					}
					closedir(dir);

		    }
		    else if(!strcmp(buffer, "put")) {
				  printf("EXECUTING: %s\n", buffer);

					// Recieve 2 files
					for(i = 0; i < 2; i++) {
						// Get fileName and piece number
						bzero(&buffer,sizeof(buffer));
						nbytes = read(clientSock, buffer, MAXBUFSIZE);

						// Create file path
						bzero(&splitInput, sizeof(splitInput));
						bzero(&filePath, sizeof(filePath));
						splitInput = strtok(buffer, " "); // fileName

						// delete files with same name
						if(i == 0) {
							bzero(&fileDeleteBuffer, sizeof(fileDeleteBuffer));
							strcat(fileDeleteBuffer, "rm ");
							strcat(fileDeleteBuffer, directoryPath);
							strcat(fileDeleteBuffer, "/.");
							printf("system command: %s\n", fileDeleteBuffer);
							strcat(fileDeleteBuffer, splitInput);
							printf("system command: %s\n", fileDeleteBuffer);
							strcat(fileDeleteBuffer, "*");
							printf("system command: %s\n", fileDeleteBuffer);
							system(fileDeleteBuffer);
						}




						strcat(filePath, directoryPath);
						strcat(filePath, "/");
						strcat(filePath, ".");
						strcat(filePath, splitInput);
						splitInput = strtok(NULL, " "); // piece number
						strcat(filePath, ".");
						strcat(filePath, splitInput);
						printf("filePath[%i]: %s\n", i, filePath);

						fd = fopen(filePath, "w+");
			  		if(fd == NULL) {
			  			printf("Error opening file...%s\n", filePath);
			  			exit(0);
			  		}

						bzero(&buffer, sizeof(buffer));
						while(1) {
			  			nbytes = read(clientSock, buffer, MAXBUFSIZE);
			  			if(!strcmp(buffer, "Over")) {
								break;
							}
			  			fprintf(fd, "%s", buffer);
			  			bzero(&buffer, sizeof(buffer));
			  		}
						printf("outside while loop\n");
						fclose(fd);
					}
		    }
		    else if(!strcmp(buffer, "list")) {
		      printf("EXECUTING: %s\n", buffer);
					dir = opendir(directoryPath);
					if(dir == NULL) {
						printf("Error opening directory\n");
						return -1;
					}

					// loop over directory
	  			bzero(&sendBuffer, sizeof(sendBuffer));
					while((in_file = readdir(dir))) {
						if(!strcmp(in_file->d_name, ".") || !strcmp(in_file->d_name, ".."))
							continue;
						printf("fileName %s\n", in_file->d_name);
						fileName = (char *) malloc(255);
						strcpy(fileName, in_file->d_name);
						fileName++;
						fileName[strlen(fileName) - 1] = '\0';
						fileName[strlen(fileName) - 1] = '\0';
						printf("fileName pointer: %s\n", fileName);
						strcat(sendBuffer, in_file->d_name);
						strcat(sendBuffer, " ");

					}
					printf("sendBuffer: %s\n", sendBuffer);
					write(clientSock, sendBuffer, MAXBUFSIZE);









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
