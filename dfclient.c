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
#include <sys/stat.h>
#include <stdbool.h>
//#include <openssl/md5.h>

#define MAXBUFSIZE 2048
#define SERVER_NUM 4
int listening = 1;

void INThandler(int sig);

typedef struct ServerConf {
  char* ip;
  char* port;
  int number;
  struct sockaddr_in remote_addr;
} ServerConf;

typedef struct SplitFile {
  char piece1[MAXBUFSIZE];
  char piece2[MAXBUFSIZE];
  char piece3[MAXBUFSIZE];
  char piece4[MAXBUFSIZE];
} SplitFile;

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
  SplitFile fileHolder;
  int fileSize = 0;
  int pieceSize1 = 0;
  int pieceSize2 = 0;
  int pieceSize3 = 0;
  int pieceSize4 = 0;
	struct stat st;
  int hashValue;

  int n;
  //MD5_CTX c;
  char buf[512];
  ssize_t bytes;

	if (argc < 2) {
		printf("USAGE: <dfc.conf> \n");
		exit(1);
	}

  // Read conf file
  readConf(argv[1], servers, &username, &password);

  // Initialize sockaddr_in structs
  for(i = 0; i < SERVER_NUM; i++) {
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
  	}

    if(connect(socks[i], (struct sockaddr *)&servers[i].remote_addr, sizeof(servers[i].remote_addr)) < 0) {
      printf("Error on connect %i.\n", i);
      close(socks[i]);
      socks[i] = -1;
    }
  }

  // Send credentials
  bzero(&sendBuffer, sizeof(sendBuffer));
  strcat(sendBuffer, username);
  strcat(sendBuffer, " ");
  strcat(sendBuffer, password);

  for(i = 0; i < SERVER_NUM; i++) {
    if(socks[i] == -1) {
      continue;
    }
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
    printf("Validated credentials...!\n\n");
  }
  else {
    printf("403!!! Incorrect credentials\n\n");
    for(i = 0; i < SERVER_NUM; i++) {
      close(socks[i]);
    }
    exit(0);
  }

	signal(SIGINT, INThandler);
	while(listening) {
    bzero(&buffer, sizeof(buffer));
    bzero(&userInput, sizeof(userInput));
    bzero(&splitInput, sizeof(splitInput));
		printf("Here is a list of commands: [get <file name>, put <file name>, list, exit]\n");
		printf("What would you like to do?\n");
		fgets(userInput, MAXBUFSIZE, stdin);
		splitInput = strtok(userInput, " ");
    printf("userInput: %s\n", userInput);

    if(!strcmp(splitInput, "get")) {
		  printf("EXECUTING: %s\n", splitInput);
      splitInput = strtok(NULL, " ");
      splitInput[strlen(splitInput) - 1] = '\0';

      for(i = 0; i < SERVER_NUM; i++) {
        write(socks[i], get, MAXBUFSIZE);
        write(socks[i], splitInput, MAXBUFSIZE);
      }


      for(i = 0; i < SERVER_NUM; i++) {
        printf("waiting on socket %i\n", i);
        for(int j = 0; j < 2; j++) {
          bzero(&buffer, sizeof(buffer));
          nbytes = read(socks[i], buffer, MAXBUFSIZE);
          printf("buffer: %s\n", buffer);
          if(!strcmp(buffer, "1")) {
            bzero(&fileHolder.piece1, sizeof(fileHolder.piece1));
            nbytes = read(socks[i], fileHolder.piece1, MAXBUFSIZE);
          }
          else if(!strcmp(buffer, "2")) {
            bzero(&fileHolder.piece2, sizeof(fileHolder.piece2));
            nbytes = read(socks[i], fileHolder.piece2, MAXBUFSIZE);
          }
          else if(!strcmp(buffer, "3")) {
            bzero(&fileHolder.piece3, sizeof(fileHolder.piece3));
            nbytes = read(socks[i], fileHolder.piece3, MAXBUFSIZE);
          }
          else if(!strcmp(buffer, "4")) {
            printf("hit\n");
            bzero(&fileHolder.piece4, sizeof(fileHolder.piece4));
            nbytes = read(socks[i], fileHolder.piece4, MAXBUFSIZE);
          }
        }
      }

      printf("piece1: %s\n", fileHolder.piece1);
      printf("piece2: %s\n", fileHolder.piece2);
      printf("piece3: %s\n", fileHolder.piece3);
      printf("piece4: %s\n", fileHolder.piece4);






    }
    else if(!strcmp(splitInput, "put")) {
		  printf("EXECUTING: %s\n", splitInput);
		  splitInput = strtok(NULL, " ");
      splitInput[strlen(splitInput) - 1] = '\0';
      printf("FileName: %s\n", splitInput);
      if(stat(splitInput, &st) == 0) {
        fileSize = st.st_size; // In bytes
        printf("File Size: %i\n", fileSize);
      }

      for(i = 0; i < SERVER_NUM; i++) {
        // Seems to not matter if here or not?
        // if(socks[i] == -1) {
        //   continue;
        // }
        write(socks[i], put, MAXBUFSIZE);
      }

      // Calculate each piece size in bytes
      pieceSize1 = fileSize / 4;
      pieceSize2 = fileSize / 4;
      pieceSize3 = fileSize / 4;
      pieceSize4 = (fileSize / 4) + (fileSize % 4);
      fd = open(splitInput,O_RDONLY);
      strcat(splitInput, " ");
      // strcat(splitInput, "1");
      // printf("FileName: %s\n", splitInput);
      // splitInput[strlen(splitInput) - 1] = '\0';
      // strcat(splitInput, "2");
      // printf("FileName: %s\n", splitInput);

      // for(i = 0; i < SERVER_NUM; i++) {
      //   write(socks[i], splitInput, MAXBUFSIZE);
      // }

      // Case fileSize % 4 == 0

      // First piece
      strcat(splitInput, "1");
      write(socks[0], splitInput, MAXBUFSIZE);
      write(socks[3], splitInput, MAXBUFSIZE);
      bzero(&sendBuffer,sizeof(sendBuffer));
      readBytes = read(fd, sendBuffer, pieceSize1);
      printf("sendBuffer1: %s\n", sendBuffer);
      write(socks[0], sendBuffer, MAXBUFSIZE);
      write(socks[3], sendBuffer, MAXBUFSIZE);
      write(socks[0], over, MAXBUFSIZE);
      write(socks[3], over, MAXBUFSIZE);

      // Second piece
      splitInput[strlen(splitInput) - 1] = '\0';
      strcat(splitInput, "2");
      write(socks[0], splitInput, MAXBUFSIZE);
      write(socks[1], splitInput, MAXBUFSIZE);
      bzero(&sendBuffer,sizeof(sendBuffer));
      readBytes = read(fd, sendBuffer, pieceSize2);
      printf("sendBuffer2: %s\n", sendBuffer);
      write(socks[0], sendBuffer, MAXBUFSIZE);
      write(socks[1], sendBuffer, MAXBUFSIZE);
      write(socks[0], over, MAXBUFSIZE);
      write(socks[1], over, MAXBUFSIZE);

      // Third piece
      splitInput[strlen(splitInput) - 1] = '\0';
      strcat(splitInput, "3");
      write(socks[1], splitInput, MAXBUFSIZE);
      write(socks[2], splitInput, MAXBUFSIZE);
      bzero(&sendBuffer,sizeof(sendBuffer));
      readBytes = read(fd, sendBuffer, pieceSize3);
      printf("sendBuffer3: %s\n", sendBuffer);
      write(socks[1], sendBuffer, MAXBUFSIZE);
      write(socks[2], sendBuffer, MAXBUFSIZE);
      write(socks[1], over, MAXBUFSIZE);
      write(socks[2], over, MAXBUFSIZE);

      // Fourth piece
      splitInput[strlen(splitInput) - 1] = '\0';
      strcat(splitInput, "4");
      write(socks[2], splitInput, MAXBUFSIZE);
      write(socks[3], splitInput, MAXBUFSIZE);
      bzero(&sendBuffer,sizeof(sendBuffer));
      readBytes = read(fd, sendBuffer, pieceSize4);
      printf("sendBuffer4: %s\n", sendBuffer);
      write(socks[2], sendBuffer, MAXBUFSIZE);
      write(socks[3], sendBuffer, MAXBUFSIZE);
      write(socks[2], over, MAXBUFSIZE);
      write(socks[3], over, MAXBUFSIZE);


    }
    else if(!strcmp(splitInput, "list\n")) {
      printf("EXECUTING: %s\n", splitInput);
    }
    else if(!strcmp(splitInput, "exit\n")) {
      printf("EXECUTING: %s\n", splitInput);
      for(i = 0; i < SERVER_NUM; i++) {
        write(socks[i], exitServer, strlen(exitServer));
        close(socks[i]);
      }
      return 1;
    }
    else {
      printf("Command unknown.\n");
    }

	}

  for(i = 0; i < SERVER_NUM; i++) {
    write(socks[i], exitServer, strlen(exitServer));
    close(socks[i]);
  }
  exit(0);
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
  bzero(&fileBuffer,sizeof(fileBuffer));
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

void INThandler(int sig)
{
	signal(sig, SIG_IGN);
	listening = 0;
	exit(0);
}
