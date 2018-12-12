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
#define FILE_LIST_SIZE 15

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

typedef struct Files {
  char name[100];
  bool piece1;
  bool piece2;
  bool piece3;
  bool piece4;
} Files;

void readConf(char* confName, ServerConf* servers, char** username, char** password);
int getFileIndex(struct Files fileList[], char* fileName);

void sigpipe_handler()
{
    printf("Connection to server lost\n");
}

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
	char list[] = "list";
	char exitServer[] = "exit";
	char over[] = "Over";
	int fd;
  FILE* fp;
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
  Files fileList[FILE_LIST_SIZE];
  int fileSize = 0;
  int pieceSize1 = 0;
  int pieceSize2 = 0;
  int pieceSize3 = 0;
  int pieceSize4 = 0;
	struct stat st;
  int hashValue;
  int pieceNum;
  int fileIndex;
  int len;
  int randomNum;
  time_t t;

  int n;
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
  signal(SIGPIPE, sigpipe_handler);
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
  printf("sendBuffer: %s\n", sendBuffer);

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


      bzero(&fileHolder.piece1, sizeof(fileHolder.piece1));
      bzero(&fileHolder.piece2, sizeof(fileHolder.piece2));
      bzero(&fileHolder.piece3, sizeof(fileHolder.piece3));
      bzero(&fileHolder.piece4, sizeof(fileHolder.piece4));
      for(i = 0; i < SERVER_NUM; i++) {
        printf("waiting on socket %i\n", i);
        for(int j = 0; j < 2; j++) {
          bzero(&buffer, sizeof(buffer));
          nbytes = read(socks[i], buffer, MAXBUFSIZE);
          printf("buffer: %s\n", buffer);
          if(!strcmp(buffer, "1")) {
            nbytes = read(socks[i], fileHolder.piece1, MAXBUFSIZE);
          }
          else if(!strcmp(buffer, "2")) {
            nbytes = read(socks[i], fileHolder.piece2, MAXBUFSIZE);
          }
          else if(!strcmp(buffer, "3")) {
            nbytes = read(socks[i], fileHolder.piece3, MAXBUFSIZE);
          }
          else if(!strcmp(buffer, "4")) {
            nbytes = read(socks[i], fileHolder.piece4, MAXBUFSIZE);
            printf("hit: %s\n", fileHolder.piece4);
          }
        }
      }

      printf("piece1: %s\n", fileHolder.piece1);
      printf("piece2: %s\n", fileHolder.piece2);
      printf("piece3: %s\n", fileHolder.piece3);
      printf("piece4: %s\n", fileHolder.piece4);

      // Check if all files are there
      if(!strlen(fileHolder.piece1) || !strlen(fileHolder.piece2) || !strlen(fileHolder.piece3) || !strlen(fileHolder.piece4)) {
        printf("File is incomplete.\n");
      }
      else {
        printf("splitInput: %s\n", splitInput);
    		fp = fopen(splitInput, "w+");
    		if(fp == NULL) {
    			printf("Error opening file...%s\n", fileName);
    			exit(-1);
    		}
        fwrite(fileHolder.piece1, 1, sizeof(fileHolder.piece1), fp);
        fwrite(fileHolder.piece2, 1, sizeof(fileHolder.piece2), fp);
        fwrite(fileHolder.piece3, 1, sizeof(fileHolder.piece3), fp);
        fwrite(fileHolder.piece4, 1, sizeof(fileHolder.piece4), fp);
        fclose(fp);
      }
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
        write(socks[i], put, MAXBUFSIZE);
      }

      srand((unsigned) time(&t));

      // Calculate each piece size in bytes
      pieceSize1 = fileSize / 4;
      pieceSize2 = fileSize / 4;
      pieceSize3 = fileSize / 4;
      pieceSize4 = (fileSize / 4) + (fileSize % 4);
      fd = open(splitInput, O_RDONLY);
      strcat(splitInput, " ");
      //printf("fileSize mod 4: %i\n", fileSize % 4);
      randomNum = rand() % 4;
      printf("random seed: %i\n", randomNum);


      // char* testString = "test";
      // bzero(&sendBuffer,sizeof(sendBuffer));
      // strcat(sendBuffer, "rm ");
      // strcat(sendBuffer, testString);
      // strcat(sendBuffer, "*");
      //
      // system(sendBuffer);

      // Send File
      switch (randomNum) {
        case 0:
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

          break;
        case 1:
          // First piece
          strcat(splitInput, "1");
          write(socks[0], splitInput, MAXBUFSIZE);
          write(socks[1], splitInput, MAXBUFSIZE);
          bzero(&sendBuffer,sizeof(sendBuffer));
          readBytes = read(fd, sendBuffer, pieceSize1);
          printf("sendBuffer1: %s\n", sendBuffer);
          write(socks[0], sendBuffer, MAXBUFSIZE);
          write(socks[1], sendBuffer, MAXBUFSIZE);
          write(socks[0], over, MAXBUFSIZE);
          write(socks[1], over, MAXBUFSIZE);

          // Second piece
          splitInput[strlen(splitInput) - 1] = '\0';
          strcat(splitInput, "2");
          write(socks[1], splitInput, MAXBUFSIZE);
          write(socks[2], splitInput, MAXBUFSIZE);
          bzero(&sendBuffer,sizeof(sendBuffer));
          readBytes = read(fd, sendBuffer, pieceSize2);
          printf("sendBuffer2: %s\n", sendBuffer);
          write(socks[1], sendBuffer, MAXBUFSIZE);
          write(socks[2], sendBuffer, MAXBUFSIZE);
          write(socks[1], over, MAXBUFSIZE);
          write(socks[2], over, MAXBUFSIZE);

          // Third piece
          splitInput[strlen(splitInput) - 1] = '\0';
          strcat(splitInput, "3");
          write(socks[2], splitInput, MAXBUFSIZE);
          write(socks[3], splitInput, MAXBUFSIZE);
          bzero(&sendBuffer,sizeof(sendBuffer));
          readBytes = read(fd, sendBuffer, pieceSize3);
          printf("sendBuffer3: %s\n", sendBuffer);
          write(socks[2], sendBuffer, MAXBUFSIZE);
          write(socks[3], sendBuffer, MAXBUFSIZE);
          write(socks[2], over, MAXBUFSIZE);
          write(socks[3], over, MAXBUFSIZE);

          // Fourth piece
          splitInput[strlen(splitInput) - 1] = '\0';
          strcat(splitInput, "4");
          write(socks[0], splitInput, MAXBUFSIZE);
          write(socks[3], splitInput, MAXBUFSIZE);
          bzero(&sendBuffer,sizeof(sendBuffer));
          readBytes = read(fd, sendBuffer, pieceSize4);
          printf("sendBuffer4: %s\n", sendBuffer);
          write(socks[0], sendBuffer, MAXBUFSIZE);
          write(socks[3], sendBuffer, MAXBUFSIZE);
          write(socks[0], over, MAXBUFSIZE);
          write(socks[3], over, MAXBUFSIZE);
          break;
        case 2:
          // First piece
          strcat(splitInput, "1");
          write(socks[1], splitInput, MAXBUFSIZE);
          write(socks[2], splitInput, MAXBUFSIZE);
          bzero(&sendBuffer,sizeof(sendBuffer));
          readBytes = read(fd, sendBuffer, pieceSize1);
          printf("sendBuffer1: %s\n", sendBuffer);
          write(socks[1], sendBuffer, MAXBUFSIZE);
          write(socks[2], sendBuffer, MAXBUFSIZE);
          write(socks[1], over, MAXBUFSIZE);
          write(socks[2], over, MAXBUFSIZE);

          // Second piece
          splitInput[strlen(splitInput) - 1] = '\0';
          strcat(splitInput, "2");
          write(socks[2], splitInput, MAXBUFSIZE);
          write(socks[3], splitInput, MAXBUFSIZE);
          bzero(&sendBuffer,sizeof(sendBuffer));
          readBytes = read(fd, sendBuffer, pieceSize2);
          printf("sendBuffer2: %s\n", sendBuffer);
          write(socks[2], sendBuffer, MAXBUFSIZE);
          write(socks[3], sendBuffer, MAXBUFSIZE);
          write(socks[2], over, MAXBUFSIZE);
          write(socks[3], over, MAXBUFSIZE);

          // Third piece
          splitInput[strlen(splitInput) - 1] = '\0';
          strcat(splitInput, "3");
          write(socks[0], splitInput, MAXBUFSIZE);
          write(socks[3], splitInput, MAXBUFSIZE);
          bzero(&sendBuffer,sizeof(sendBuffer));
          readBytes = read(fd, sendBuffer, pieceSize3);
          printf("sendBuffer3: %s\n", sendBuffer);
          write(socks[0], sendBuffer, MAXBUFSIZE);
          write(socks[3], sendBuffer, MAXBUFSIZE);
          write(socks[0], over, MAXBUFSIZE);
          write(socks[3], over, MAXBUFSIZE);

          // Fourth piece
          splitInput[strlen(splitInput) - 1] = '\0';
          strcat(splitInput, "4");
          write(socks[0], splitInput, MAXBUFSIZE);
          write(socks[1], splitInput, MAXBUFSIZE);
          bzero(&sendBuffer,sizeof(sendBuffer));
          readBytes = read(fd, sendBuffer, pieceSize4);
          printf("sendBuffer4: %s\n", sendBuffer);
          write(socks[0], sendBuffer, MAXBUFSIZE);
          write(socks[1], sendBuffer, MAXBUFSIZE);
          write(socks[0], over, MAXBUFSIZE);
          write(socks[1], over, MAXBUFSIZE);

          break;
        case 3:
          // First piece
          strcat(splitInput, "1");
          write(socks[2], splitInput, MAXBUFSIZE);
          write(socks[3], splitInput, MAXBUFSIZE);
          bzero(&sendBuffer,sizeof(sendBuffer));
          readBytes = read(fd, sendBuffer, pieceSize1);
          printf("sendBuffer1: %s\n", sendBuffer);
          write(socks[2], sendBuffer, MAXBUFSIZE);
          write(socks[3], sendBuffer, MAXBUFSIZE);
          write(socks[2], over, MAXBUFSIZE);
          write(socks[3], over, MAXBUFSIZE);

          // Second piece
          splitInput[strlen(splitInput) - 1] = '\0';
          strcat(splitInput, "2");
          write(socks[0], splitInput, MAXBUFSIZE);
          write(socks[3], splitInput, MAXBUFSIZE);
          bzero(&sendBuffer,sizeof(sendBuffer));
          readBytes = read(fd, sendBuffer, pieceSize2);
          printf("sendBuffer2: %s\n", sendBuffer);
          write(socks[0], sendBuffer, MAXBUFSIZE);
          write(socks[3], sendBuffer, MAXBUFSIZE);
          write(socks[0], over, MAXBUFSIZE);
          write(socks[3], over, MAXBUFSIZE);

          // Third piece
          splitInput[strlen(splitInput) - 1] = '\0';
          strcat(splitInput, "3");
          write(socks[0], splitInput, MAXBUFSIZE);
          write(socks[1], splitInput, MAXBUFSIZE);
          bzero(&sendBuffer,sizeof(sendBuffer));
          readBytes = read(fd, sendBuffer, pieceSize3);
          printf("sendBuffer3: %s\n", sendBuffer);
          write(socks[0], sendBuffer, MAXBUFSIZE);
          write(socks[1], sendBuffer, MAXBUFSIZE);
          write(socks[0], over, MAXBUFSIZE);
          write(socks[1], over, MAXBUFSIZE);

          // Fourth piece
          splitInput[strlen(splitInput) - 1] = '\0';
          strcat(splitInput, "4");
          write(socks[1], splitInput, MAXBUFSIZE);
          write(socks[2], splitInput, MAXBUFSIZE);
          bzero(&sendBuffer,sizeof(sendBuffer));
          readBytes = read(fd, sendBuffer, pieceSize4);
          printf("sendBuffer4: %s\n", sendBuffer);
          write(socks[1], sendBuffer, MAXBUFSIZE);
          write(socks[2], sendBuffer, MAXBUFSIZE);
          write(socks[1], over, MAXBUFSIZE);
          write(socks[2], over, MAXBUFSIZE);

          break;
        default:;
      }

      close(fd);




    }
    else if(!strcmp(splitInput, "list\n")) {
      printf("EXECUTING: %s\n", splitInput);
      for(i = 0; i < SERVER_NUM; i++) {
        // Seems to not matter if here or not?
        printf("inside loop\n");
        // if(socks[i] == -1) {
        //   printf("hit\n");
        //   continue;
        // }
        len = write(socks[i], list, MAXBUFSIZE);
        if(len < 0) {
          printf("error\n");
          continue;
        }
        printf("after write\n");
        if(errno == ENOTCONN) {
          printf("no connection\n");
        }
      }
      printf("after first for loop\n");

      // reset file pieces
      for(i = 0 ; i < FILE_LIST_SIZE; i++) {
        fileList[i].piece1 = false;
        fileList[i].piece2 = false;
        fileList[i].piece3 = false;
        fileList[i].piece4 = false;
      }

      for(i = 0; i < SERVER_NUM; i++) {
        if(socks[i] == -1) {
          continue;
        }
        bzero(&buffer, sizeof(buffer));
        bzero(&splitInput, sizeof(splitInput));
        nbytes = read(socks[i], buffer, MAXBUFSIZE);
        splitInput = strtok(buffer, " ");
        while(splitInput != NULL) {
          pieceNum = atoi(&splitInput[strlen(splitInput) - 1]);
          splitInput++;
          splitInput[strlen(splitInput) - 1] = '\0';
          splitInput[strlen(splitInput) - 1] = '\0';

          //get index
          fileIndex = getFileIndex(fileList, splitInput);
          switch (pieceNum) {
            case 1:
              fileList[fileIndex].piece1 = true;
              break;
            case 2:
              fileList[fileIndex].piece2 = true;
              break;
            case 3:
              fileList[fileIndex].piece3 = true;
              break;
            case 4:
              fileList[fileIndex].piece4 = true;
              break;
          }
          splitInput = strtok(NULL, " ");
        }
      }

      for(i = 0; i < FILE_LIST_SIZE; i++) {
        if(strlen(fileList[i].name) <=0) {
          continue;
        }
        else if(fileList[i].piece1 && fileList[i].piece2 && fileList[i].piece3 && fileList[i].piece4) {
          printf("%s\n", fileList[i].name);
        }
        else {
          printf("%s [Incomplete]\n", fileList[i].name);
        }
      }

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

int getFileIndex(struct Files fileList[], char* fileName)
{

  int i;

  for(i = 0; i < FILE_LIST_SIZE; i++) {
    printf("fileName: %s\n", fileList[i].name);
    if(strlen(fileList[i].name) <= 0) {
      printf("hit1\n");
      strcpy(fileList[i].name, fileName);
      return i;
    }
    else if(!strcmp(fileList[i].name, fileName)) {
      return i;
    }


  }

  return -1;

}
