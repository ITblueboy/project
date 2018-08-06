#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<fcntl.h>
#include<sys/types.h>
#include<signal.h>

#define SIZE (1024*4)

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

typedef struct HttpRequest
{
  char first_line[SIZE];
  char *method;
  char *url;
  char *url_path;
  int content_length;
  char *query_string;
}HttpRequest;



void HandleRequest(int64_t new_sock);
int ParseHeader(int new_sock,int* content_length);
