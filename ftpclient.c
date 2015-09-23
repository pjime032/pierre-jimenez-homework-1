#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>

void syserr(char* msg) { perror(msg); exit(-1); }
void recvandwrite(int tempfd, int sockfd, int size, char* buffer);

int main(int argc, char* argv[])
{
  //Sockfd: file descriptor; portno: port number; n: return values for read and write
  int sockfd, portno, n, size, tempfd;
  char input[50];
  char *file; //user input, file pointer
  struct hostent* server; // server info
  struct sockaddr_in serv_addr; //server address info
  char buffer[256];

  if(argc != 3) {
    fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
    return 1;
  }
  server = gethostbyname(argv[1]);
  if(!server) {
    fprintf(stderr, "ERROR: no such host: %s\n", argv[1]);
    return 2;
  }
  portno = atoi(argv[2]);

  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // check man page
  if(sockfd < 0) syserr("can't open socket");
  printf("create socket...\n");

 // set all to zero, then update the sturct with info
  memset(&serv_addr, 0, sizeof(serv_addr)); 
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr = *((struct in_addr*)server->h_addr);
  serv_addr.sin_port = htons(portno); 	

 // connect with filde descriptor, server address and size of addr
  if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    syserr("can't connect to server");
  printf("connect...\n");
  printf("Connection established. Waiting for commands...\n");
  
  for(;;){
  	scanf("%s", input);
  	if(strcmp(input, "ls-local") == 0)
  	{
		system("ls -a | cat");
  	}
  	if(strcmp(input, "ls-remote") == 0)
  	{
  		memset(buffer, 0, sizeof(buffer));
		strcpy(buffer, "ls");
		n = send(sockfd, buffer, strlen(buffer), 0); // may need to fix buffer with /0 ending
		if(n < 0) syserr("can't send to server");
		n = recv(sockfd, &size, sizeof(int), 0);
		size = ntohl(size);
        if(n < 0) syserr("can't receive from server");
		file = malloc(size); // make space for a list of files on the server
		
		n = recv(sockfd, file, sizeof(file), 0); //put list into f
		tempfd = creat("remotelist.txt", 0666); //create  a text file to put list
		write(tempfd, &file, size);
		close(tempfd);
		printf("Files at the server(%s)\n", argv[1]);
		system("cat remotelist.txt");
		free(file);		
  	}
  	if(strcmp(input, "exit") == 0)
  	{
  		memset(buffer, 0, sizeof(buffer));
		strcpy(buffer, "exit");
		printf("sending command to server: %s\n", buffer);
		n = send(sockfd, buffer, strlen(buffer), 0); // may need to fix buffer with /0 ending
		printf("amount of data sent: %d\n", n);
		if(n < 0) syserr("can't send to server");
		n = recv(sockfd, &size, sizeof(int), 0);
        size = ntohl(size);  
        if(n < 0) syserr("can't receive from server");
        
		printf("size was %d from server\n", size);
		if(size)
		{
			exit(0);
		}
		else
		{
			printf("Server didn't exit");
		}
		
  	} 	
  	//tokenize if get/put
  	char *comm, *filename;
  	comm = strtok(input, " ");
  	filename = strtok(NULL, " ");
	
  	if(strcmp(comm, "get") == 0)
  	{
  		strcpy(buffer, "get");
  		strcat(buffer, filename);
		n = send(sockfd, buffer, strlen(buffer), 0); // may need to fix buffer with /0 ending
		if(n < 0) syserr("can't send to server");
		n = recv(sockfd, &size, sizeof(int), 0); // get the file pointer
        if(n < 0) syserr("can't receive from server");
        size = ntohl(size);        
		if(size ==0) // check if file exists
		{
			printf("File not found\n");
			break;
		}
		tempfd = open(filename, O_CREAT | O_WRONLY, 0666);
		recvandwrite(tempfd, sockfd, size, buffer);
		
		/*    CHANGE ALL THIS!!!
		file = malloc(size);
		n = recv(sockfd, file, sizeof(file), 0); // receieve the file
		tempfd = open(filename, O_CREAT | O_WRONLY, 0666); //overwrites exisitng file??
		write(tempfd, &file, size);
		close(tempfd);
		*/
  	}
  	if(strcmp(comm, "put") == 0)
  	{
  		strcpy(buffer, "put");
  		strcat(buffer, filename);
  	}
  	
  }
  close(sockfd);
  return 0;
}

void recvandwrite(int tempfd, int sockfd, int size, char* buffer)
{
	int totalWritten = 0;
	while(1)
	{
		int total = 0;
		int bytesleft = sizeof(buffer); //bytes left to recieve
		int n;
		while(total < sizeof(buffer))
		{
			n = recv(sockfd, buffer+total, bytesleft, 0);
			if (n == -1) 
			{ 
				syserr("error sending file"); 
				break;
			}
			total += n;
			bytesleft -= n;
		}
		
		int bytes_written = write(tempfd, buffer, sizeof(buffer)); //is buffer cleared here?
		totalWritten += bytes_written;
		if (bytes_written == 0 || totalWritten == size) // We're done writing into the file
			break;

		if (bytes_written < 0) syserr("error writing file"); 
    }	
}