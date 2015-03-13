/* 
This code primarily comes from 
http://www.prasannatech.net/2008/07/socket-programming-tutorial.html
and
http://www.binarii.com/files/papers/c_sockets.txt
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

int PORT_NUMBER;
char* input;
char in[256];
pthread_mutex_t lock;
int flag = 1;
int successful_counter = 0;
int not_handled = 0;
int bytes_sent = 0;

void* start_server()
{

      // structs to represent the server and client
      struct sockaddr_in server_addr,client_addr;    
      
      int sock; // socket descriptor

      // 1. socket: creates a socket descriptor that you later use to make other system calls
      if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("Socket");
	exit(1);
      }
      int temp;
      if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&temp,sizeof(int)) == -1) {
	perror("Setsockopt");
	exit(1);
      }

      // configure the server
      server_addr.sin_port = htons(PORT_NUMBER); // specify port number
      server_addr.sin_family = AF_INET;         
      server_addr.sin_addr.s_addr = INADDR_ANY; 
      bzero(&(server_addr.sin_zero),8); 
      
      // 2. bind: use the socket and associate it with the port number
      if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
	perror("Unable to bind");
	exit(1);
      }

      // 3. listen: indicates that we want to listen to the port to which we bound; second arg is number of allowed connections
      if (listen(sock, 1) == -1) {
	perror("Listen");
	exit(1);
      }
          
      // once you get here, the server is set up and about to start listening
      printf("\nServer configured to listen on port %d\n", PORT_NUMBER);
      fflush(stdout);
      char request[1024];
      char *reply = (char *)malloc(sizeof(char) * (1500));
      char *reply2 = (char *)malloc(sizeof(char) * (1500));
      while(flag){
          pthread_mutex_lock(&lock);

          // 4. accept: wait here until we get a connection on that port
          int sin_size = sizeof(struct sockaddr_in);
          int fd = accept(sock, (struct sockaddr *)&client_addr,(socklen_t *)&sin_size);
          printf("Server got a connection from (%s, %d)\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
          
          // buffer to read data into
    //      char request[1024];
          // 5. recv: read incoming message into buffer
        
          int bytes_received = recv(fd,request,1024,0);
          // null-terminate the string
          request[bytes_received] = '\0';

          // this is the message that we'll send back
          // for now, it's just a copy of what we received
        
          char s[2] = " ";
          char* head;
          char* requestedFile;
          char* requestedFile2;
          char fileAbsolutePath[500];
          if(reply == NULL || reply2 == NULL){
              strcpy(reply2, "HTTP/1.1 500 Internal Server Error\nContent-Type: text/html\n\n");
              strcat(reply2, "<HTML>\r\n<HEAD><TITLE>Internal Server Error</TITLE></HRAD>\r\n");
              strcat(reply2, "<H1>HTTP Error 500: ");
              strcat(reply2, "Internal Server Error</H1></HTML>\r\n");
              not_handled++;
          }
          else{
              reply[0] = '\0';
              reply2[0] = '\0';
              head = strtok(request, s);
              requestedFile = strtok(NULL, s);
              if(strcmp(requestedFile, "/stats") == 0){
                  char buffer[bytes_sent + 10];
                  strcpy(reply2, "HTTP/1.1 200 OK\nContent-Type: text/html\n\n");
                  strcat(reply2, "<HTML>\r\n<HEAD><TITLE>Statistics</TITLE></HRAD>\r\n");
                  strcat(reply2, "<UL><LI>Number of page requests handled successfully: ");
                  sprintf(buffer, "%d", successful_counter);
                  strcat(reply2, buffer);
                  strcat(reply2, "</LI>\r\n");
                  strcat(reply2, "<LI>Number of page requests not handled: ");
                  sprintf(buffer, "%d", not_handled);
                  strcat(reply2, buffer);
                  strcat(reply2, "</LI>\r\n");
                  strcat(reply2, "<LI>Number of bytes sent back: ");
                  sprintf(buffer, "%d", bytes_sent);
                  strcat(reply2, buffer);
                  strcat(reply2, "</LI></UL></HTML>\r\n");
                  
              }
              else{
                  strcpy(fileAbsolutePath, input);
                  strcat(fileAbsolutePath, requestedFile);
                  printf("Absolute Path: %s\n", fileAbsolutePath);
                  FILE * infile;
                  infile = fopen(fileAbsolutePath, "rb");
                  if(infile == NULL) {
                      strcpy(reply2, "HTTP/1.1 404 File Not Found\nContent-Type: text/html\n\n");
                      strcat(reply2, "<HTML>\r\n<HEAD><TITLE>File Not Found</TITLE></HRAD>\r\n");
                      strcat(reply2, "<H1>HTTP Error 404: ");
                      strcat(reply2, requestedFile);
                      strcat(reply2, " Not Found</H1></HTML>\r\n");
                      not_handled++;
                  }
                  else{
                      strcpy(reply2, "HTTP/1.1 200 OK\nContent-Type: text/html\n\n");
                      if(strcmp(requestedFile, "/") == 0){
                          strcat(reply2, "<HTML>\r\n<HEAD><TITLE>Root Directory</TITLE></HRAD>\r\n");
                          strcat(reply2, "<H1> root directory</H1></HTML>\r\n");
                          successful_counter++;
                      }
                      else{
                          fseek(infile, 0, SEEK_END);
                          int length = ftell(infile);
                          fseek(infile, 0, SEEK_SET);
                          if(infile!= '\0'){
                            fread(reply, 1, length, infile);
                          }
                          strcat(reply2, reply);
                          successful_counter++;
                      }
                  }
              }
          }
          // 6. send: send the message over the socket
          // note that the second argument is a char*, and the third is the number of chars
          bytes_sent = bytes_sent + strlen(reply2);
          send(fd, reply2, strlen(reply2), 0);
          printf("Server sent message: %s\n", reply2);
          close(fd);
          pthread_mutex_unlock(&lock);

      }
      // 7. close: close the socket connection
      close(sock);
      free(reply);
      free(reply2);
      printf("Server closed connection\n");
      pthread_exit(NULL);
      exit(0);

}

int main(int argc, char *argv[])
{
  // check the number of arguments
  if (argc != 3)
    {
      printf("\nUsage: server [port_number]\n");
      exit(0);
    }
  
  PORT_NUMBER = atoi(argv[1]);
  input = argv[2];
  pthread_t t1;
  pthread_mutex_init(&lock, NULL);
  pthread_create(&t1, NULL, &start_server, NULL);
  void*r1;
  while(1){
      printf("Enter input:");
      fgets(in, 256, stdin);
      if(strcmp(in, "q\n") == 0){
          flag = 0;
          break;
      }
  }
 pthread_join(t1, &r1);
 return 0;
}

