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
#define SERVER_NUM 4

typedef struct ServerConf {
  char* ip;
  char* port;
  int number;
  struct sockaddr_in remote_addr;
} ServerConf;

void readConf(char* confName, ServerConf* servers, char** username, char** password);

int main (int argc, char * argv[])
{

	int nbytes, readBytes;                             // number of bytes send by sendto()
	int socks[SERVER_NUM];                               //this will be our socket
	char buffer[MAXBUFSIZE];
	char fileBuffer[MAXBUFSIZE];
	char sendBuffer[MAXBUFSIZE];
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
  int i;
  bool authenticated = false;
  FILE* confFD;
	char* splitInput;
  char* saveptr;
  char* username;
  char* password;
  char* ip;
  ServerConf servers[SERVER_NUM];

	if (argc < 2) {
		printf("USAGE: <dfc.conf> \n");
		exit(1);
	}

  // Read conf file
  readConf(argv[1], servers, &username, &password);

  // Initialize sockaddr_in structs
  for(i = 0; i < SERVER_NUM; i++) {
    printf("number: %i\n", servers[i].number);
    printf("ip: %s\n", servers[i].ip);
    printf("port: %s\n", servers[i].port);
    bzero(&servers[i].remote_addr, sizeof(servers[i].remote_addr));
    servers[i].remote_addr.sin_family = AF_INET;
    servers[i].remote_addr.sin_port = htons(atoi(servers[i].port));
    servers[i].remote_addr.sin_addr.s_addr = inet_addr(servers[i].ip);
  }

	// bzero(&remote_addr,sizeof(remote_addr));               //zero the struct
	// remote_addr.sin_family = AF_INET;                 //address family
	// remote_addr.sin_port = htons(10001);      //sets port to network byte order
	// remote_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //sets remote IP address

  // Setup connections to each server
  for(i = 0; i < SERVER_NUM; i++) {
    if ((socks[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
  		printf("unable to create socket");
      return -1;
  	}

    if(connect(socks[i], (struct sockaddr *)&servers[i].remote_addr, sizeof(servers[i].remote_addr)) < 0) {
      printf("Error on connect %i.\n", i);
      return -1;
    }
  }

  // Send credentials
  bzero(&sendBuffer, sizeof(sendBuffer));
  strcat(sendBuffer, username);
  strcat(sendBuffer, " ");
  strcat(sendBuffer, password);

  for(i = 0; i < SERVER_NUM; i++) {
    bzero(&buffer, sizeof(buffer));
    write(socks[i], sendBuffer, strlen(sendBuffer));
    nbytes = read(socks[i], buffer, MAXBUFSIZE);
    if(!strcmp(buffer, "VALID")) {
      authenticated = true;
    }
    else {
      authenticated = false;
    }
  }
  if(authenticated) {
    printf("AUTH WORKS!\n");
  }


	// while(1) {
	// 	printf("\n\nHere is a list of commands: [get, put, ls, delete, exit]\n");
	// 	printf("What would you like to do?\n");
	// 	fgets(userInput, MAXBUFSIZE, stdin);
	// 	splitInput = strtok(userInput, " ");
	// 	printf("EXECUTING: %s\n", splitInput);
  //   write(sock, get, strlen(get));
  //   nbytes = read(sock, buffer, MAXBUFSIZE);
  //   printf("BUFFER: %s\n", buffer);
  //
	// }

  for(i = 0; i < SERVER_NUM; i++) {
    close(socks[i]);
  }
}

void readConf(char* confName, ServerConf* servers, char** username, char** password)
{

  FILE* confFD;
  char* saveptr;
	char* splitInput;
  char* ip;
  char fileBuffer[MAXBUFSIZE];
  confFD = fopen(confName, "r");
  if(confFD == NULL) {
    printf("Error opening file.\n");
    exit(0);
  }
  bzero(fileBuffer,sizeof(fileBuffer));
  int serverNum;
  while(1) {
    if(fgets(fileBuffer, sizeof(fileBuffer), confFD)!=NULL ) {
      if(!strncmp(&fileBuffer[0], "U", 1)) {
        splitInput = strtok_r(fileBuffer, " ", &saveptr);
        splitInput = strtok_r(NULL, " ", &saveptr);
        *username = (char*) malloc(strlen(splitInput));
        splitInput[strlen(splitInput) - 1] = '\0';
        strcpy(*username, splitInput);
      }
      else if(!strncmp(&fileBuffer[0], "P", 1)) {
        splitInput = strtok_r(fileBuffer, " ", &saveptr);
        splitInput = strtok_r(NULL, " ", &saveptr);
        *password = (char*) malloc(strlen(splitInput));
        splitInput[strlen(splitInput) - 1] = '\0';
        strcpy(*password, splitInput);
      }
      else {
        splitInput = strtok_r(fileBuffer, " ", &saveptr);
        serverNum = atoi(&splitInput[strlen(splitInput) - 1]);
        servers[serverNum - 1].number = serverNum;
        splitInput = strtok_r(NULL, " ", &saveptr);
        ip = strtok_r(splitInput, ":", &saveptr);
        servers[serverNum - 1].ip = (char *) malloc(strlen(ip));
        strcpy(servers[serverNum - 1].ip, ip);
        ip = strtok_r(NULL, ":", &saveptr);
        servers[serverNum - 1].port = (char *) malloc(strlen(ip));
        ip[strlen(ip) - 1] = '\0';
        strcpy(servers[serverNum - 1].port, ip);
      }
    }
    else {
      break;
    }
  }

}
