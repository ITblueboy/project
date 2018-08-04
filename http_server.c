#include"http_server.h"

ssize_t ReadLine(int64_t sock,char output[],ssize_t max_size)
{
  //1.从socket读取每一个字符
  char c='\0';
  ssize_t i=0;
  while(i<max_size)
  {
    ssize_t read_size=recv(sock,&c,1,0);
    if(read_size<=0)
    {
      return -1;
    }
    if(c=='\r')
    {
      recv(sock,&c,1,MSG_PEEK);
      if(c=='\n')
      {
        recv(sock,&c,1,0);
      }
      else 
      {
        c='\n';
      }
    }

    if(c=='\n')
    {
      break;
    }
    output[i++]=c;
  }
  output[i++]='\0';
  return 1;
  //2.判定当前字符是不是\r
  //3.如果当前字符是\r，读取下一个字符
  //a.下一个字符是\r
  //b.下一个字符不是\n
  //都将当前字符转为\n
  //4.如果当前字符是\n，表示第一行读取完毕，结束函数
  //5.如果当前字符是普通字符，直接输出到结果中
}

int Split(char input[],const char* split_char,char* output[])
{
  int output_index=0;
  char* tmp=NULL;
  char*p=strtok_r(input,split_char,&tmp);
  while(p!=NULL)
  {
    output[output_index++]=p;
    p=strtok_r(NULL,split_char,&tmp);
  }
  return output_index;
}

//此写法没考虑域名，后续追加
int ParseFirstLine(char first_line[],char** p_method,char** p_url)
{
  char* tok[10]={0};
  int n=Split(first_line," ",tok);
  if(n!=3)
  {
    printf("Split切分失败,%d\n",n);
    return -1;
  }

  *p_method=tok[0];
  *p_url=tok[1];
  return 0;
}

void HandleRequest(int64_t new_sock)
{
  //1.解析请求
  //a.从socket中读出首行
  HttpRequest req;
  memset(&req,0,sizeof(req));
  if(ReadLine(new_sock,req.first_line,sizeof(req.first_line)-1)<0)
  {

  }
  printf("first line:%s\n",req.first_line);
  //b.解析首行，获取到URL和method
  if(ParseFirstLine(req.first_line,&req.method,&req.url)<0)
  {

  }
  //c.解析URL，获取到URL_Path和query_string
  if(ParserUrl(req.url,&req.url_path,&req.query_string)<0)
  {

  }
  //d.解析header，丢弃大部分header,只保留content-Length
  if(ParseHeader(new_sock,&req.content_length))
  {

  }

  //2.根据请求计算响应并写会客户端
  if(strcasecmp(req.method,"GET")==0&&req.query_string==NULL)
  {

  }
  //a.处理静态页面
  else if(strcasecmp(req.method,"GET")==0&&req.query_string!=NULL)
  {
    HandleStatic();
  }
  //b.处理动态页面
  else if(strcasecmp(req.method,"POST")==0)
  {
    HandleCGI(); 
  }
  //其他方法请求
  else 
  {

  }
}
void* ThreadEntry(void* arg)
{
  int64_t new_sock=(int64_t)arg;
  HandleRequest(new_sock);
  return NULL;
}

void http_server_start(char* ip,char* port)
{
  int lisetn_sock=socket(AF_INET,SOCK_STREAM,0);
  if(lisetn_sock<0)
  {
    perror("socket perror\n");
    return;
  }

  int opt=1;
  setsockopt(lisetn_sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));


  sockaddr_in addr;
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=inet_addr(ip);
  addr.sin_port=htons(atoi(port));

  int ret=bind(lisetn_sock,(sockaddr*)&addr,sizeof(addr));
  if(ret<0)
  {
    perror("bind error\n");
    return;
  }

  ret=listen(lisetn_sock,5);
  if(ret<0)
  {
    perror("listen error\n");
    return;
  }

  printf("http_server start\n");

  while(1)
  {
    sockaddr_in peer;
    socklen_t len=sizeof(peer);
    int64_t new_sock=accept(lisetn_sock,(sockaddr*)& peer,&len);
    if(new_sock<0)
    {
      perror("accept\n");
      return ;
    }

    pthread_t tid;
    pthread_create(&tid,NULL,ThreadEntry,(void*)new_sock);
    //线程分离
    pthread_detach(tid);
  }
}

int main(int argc,char* argv[])
{
  if(argc!=3)
  {
    printf("参数个数不正确\n");
    return 1;
  }
  http_server_start(argv[1],argv[2]);
  return 0;
}
