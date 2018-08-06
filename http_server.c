#include"http_server.h"

ssize_t ReadLine(int64_t sock,char output[],ssize_t max_size)
{
  //1.从socket读取每一个字符
  printf("ReadLine start\n");
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
  printf("ReadLine finish\n");
  return 0;
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

int ParserUrl(char url[],char** p_url_path,char** p_query_string)
{
  *p_url_path=url;
  char* p=url;
  for(;*p!='\0';++p)
  {
    if(*p=='?')
    {
    *p='\0';
    *p_query_string=p+1;
    return 0;
    }
  }
  *p_query_string=NULL;
  return 0;
}

int ParseHeader(int new_sock,int* content_length)
{
  printf("ParseHeader start\n");
  char buf[SIZE]={0};
  while(1)
  {
    ssize_t read_size=ReadLine(new_sock,buf,sizeof(buf)-1);
    if(read_size<0)
    {
      return -1;
    }
    if(strcmp(buf,"\0")==0)
    {
      return 0;
    }
    const char* src="Content-Length: ";
    if(strncmp(buf,src,strlen(src))==0)
    {
      //此处不能使用break；
      *content_length=atoi(buf+strlen(src));
    }
  }
  printf("ParseHeader finish\n");
}

void Handler_error(int64_t new_sock)
{
  printf("%ld\n",new_sock);
  printf("error\n");
  const char* first_line="HTTP/1.1 404 Not Found\n";
  const char* body="<head><meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\"></head>"
                   "<h1>页面找不到了 </h1>";
  const char* blank="\n";
  char Content_length[SIZE]={0};
  sprintf(Content_length, "Content-Length: %lu\n", strlen(body));
  printf("error handle,finish1\n");
  send((int)new_sock,first_line,strlen(first_line),0);
  send(new_sock,Content_length,strlen(Content_length),0);
  send((int)new_sock,blank,strlen(blank),0);
  send((int)new_sock,body,strlen(body),0);
  printf("error handle,finish2\n");
  return;
}

int IsDir(const char* file_path)
{
  struct stat st;
  int ret=stat(file_path,&st);
  if(ret<0)
  {
    return 0;
  }
  if(S_ISDIR(st.st_mode))
  {
    return 1;
  }
  return 0;
}

void HandlerFilePath(const char* url_path,char file_path[])
{
  //URL_path是以/开头的，所以不需要显示指明
  sprintf(file_path,"./src%s",url_path);
  //如果url_path指向的目录，就在目录后面拼接上访问的文件
  //a.URL_path以/结尾，为目录
  if(file_path[strlen(file_path)-1]=='/')
  {
    strcat(file_path,"index.html");
  }
  //b.不以/结尾，需要依靠属性判断
  else
  {
    if(IsDir(file_path))
    {
      strcat(file_path,"index.html");
    }
  }
}

ssize_t GetFileSize(const char* file_path)
{
  struct stat st;
  int ret=stat(file_path,&st);
  if(ret<0)
  {
    return 0;
  }
  return st.st_size;
}

int WriteStaticFile(int new_sock,const char* file_path )
{
  //1.打开文件，打开失败，返回404
  int fd=open(file_path,O_RDONLY);
  //构造http响应
  const char* first_line="HTTP/1.1 200 OK\n";
  const char* blank_line="\n";
  send(new_sock,first_line,strlen(first_line),0);
  send(new_sock,blank_line,strlen(blank_line),0);
  //3.读文件并写到socket
  ssize_t file_size=GetFileSize(file_path);
  sendfile(new_sock,fd,NULL,file_size);
  //4.关闭文件
  close(fd);
  return 200;
}

int HandleStatic(int new_sock,const HttpRequest* req)
{
  //1.根据URL_path获取到文件的真实路径
  //例如，有一个服务器的根目录为src
  //在URL写上文件的路径
  char file_path[SIZE]={0};
  //根据下面的函数把URL上的路径转为磁盘路径
  HandlerFilePath(req->url_path,file_path);
  //2.打开文件，读取文件内容，把文件内容写到socket中
  int error_code=WriteStaticFile(new_sock,file_path);
  return error_code;
}

void HandleChild(int child_read,int child_write,const HttpRequest* req)
{
  
  //4.子进程核心流程
  //a.设置环境变量（METHOD，QUERY_STRING,CONTENT_LENGTH),
  //如果把上面这几个信息通过管道来告知替换之后的程序，
  //也是可以的，但是这里要遵守CGI标准，所以要通过环境变量传递信息；
  char method_env[SIZE]={0};
  sprintf(method_env,"REQUEST_METHOD=%s",req->method);
  putenv(method_env);

  if(strcasecmp(req->method,"GET")==0)
  {
    char query_string_env[SIZE]={0};
    sprintf(query_string_env,"QUERY_STRING=%s",req->query_string);
    putenv(query_string_env);
  }
  else 
  {
    char content_lenth_env[SIZE]={0};
    sprintf(content_lenth_env,"CONTENT_LENTH=%d",req->content_length);
    putenv(content_lenth_env);
  }
  //b.把标准输入和标准输出重定向到管道上；此时CGI程序读写标准输入和输出相当于读写管道
  dup2(child_read,0);
  dup2(child_write,1);
  //c.子进程进行程序替换（需要先找到是哪个CGI可执行程序，然后在使用exec函数进行替换）；
  //替换成功后,动态页面交由CGI生成；
  char file_path[SIZE]={0};
  HandlerFilePath(req->url_path,file_path);
  execl(file_path,file_path,NULL);
  //替换失败的
  exit(0);
}

int HandleFather(int new_sock,int father_read,int father_write,const HttpRequest* req)
{
  //3.父进程核心流程
  //a.如果是POST请求，把body部分的数据读出来写到管道中，
  //剩下的动态生成页面的过程都交给子进程来完成
  if(strcasecmp(req->method,"POST")==0)
  {
    //根据body长度决定读取字节数
    char c='\0';
    int i=0;
    for(;i<req->content_length;i++)
    {
      read(new_sock,&c,1);
      write(father_write,&c,1);
    }
  }
  //b.构造HTTP响应的首行，header，空行
  const char* first_line="HTTP/1.1 200 OK\n";
  send(new_sock,first_line,strlen(first_line),0);
  const char* blank_line="\n";
  send(new_sock,blank_line,strlen(blank_line),0);
  //c.从管道中读取数据（子进程动态生成的页面），把这个数据也写到socket中
  char c='\n';
  while(read(father_read,&c,1)>0)
  {
    write(new_sock,&c,1);
  }
  //d.进程等待，回收子进程的资源
  return 200;
}

int HandleCGI(int new_sock,HttpRequest req)
{
	//1.创建一对匿名管道
  int fd1[2],fd2[2];
  pipe(fd1);
  pipe(fd2);

  int father_read=fd1[0];
  int father_write=fd2[1];
  int child_read=fd1[1];
  int child_write=fd2[0];

  //2.创建子进程fork
  pid_t ret=fork();
  if(ret>0)
  {
    //此处将不必要的文件描述符关闭，为了保证后面
    close(child_read);
    close(child_write);
    HandleFather(new_sock,father_read,father_write,&req);
  }
  else if(ret==0)
  {
    close(father_read);
    close(father_write);
    HandleChild(child_read,child_write,&req);
  }
  else 
  {
    perror("fork error\n");
    goto END;
  }
END:
  close(father_read);
  close(father_write);
  close(child_read);
  close(child_write);
  return 404;
}

void HandleRequest(int64_t new_sock)
{
  //1.解析请求
  //a.从socket中读出首行
  HttpRequest req;
  memset(&req,0,sizeof(req));
  int error_code=200;
  if(ReadLine(new_sock,req.first_line,sizeof(req.first_line)-1)<0)
  {
    printf("ReadLine error\n");
    error_code=404;
    goto END;
  }
  printf("first line:%s\n",req.first_line);
  //b.解析首行，获取到URL和method
  if(ParseFirstLine(req.first_line,&req.method,&req.url)<0)
  {
    printf("ParseFirstLine error\n");
    error_code=404;
    goto END;

  }
  //c.解析URL，获取到URL_Path和query_string
  if(ParserUrl(req.url,&req.url_path,&req.query_string)<0)
  {
    printf("ParserUrl error\n");
    error_code=404;
    goto END;

  }
  //d.解析header，丢弃大部分header,只保留content-Length
  if(ParseHeader(new_sock,&req.content_length)<0)
  {
    printf("ParseHeader error\n");
    error_code=404;
    goto END;

  }

  //2.根据请求计算响应并写会客户端
  if(strcasecmp(req.method,"GET")==0&&req.query_string==NULL)
  {
    error_code=HandleStatic(new_sock,&req);

  }
  //a.处理静态页面
  else if(strcasecmp(req.method,"GET")==0&&req.query_string!=NULL)
  {
    error_code=HandleStatic(new_sock,&req);
  }
  //b.处理动态页面
  else if(strcasecmp(req.method,"POST")==0)
  {
    error_code=HandleCGI(new_sock,req); 
  }
  //其他方法请求
  else 
  {
    error_code=404;
    goto END;

  }
END:
  if(error_code!=200)
  {
    printf("error handle\n");
    Handler_error(new_sock);
  }
  close(new_sock);
}
void* ThreadEntry(void* arg)
{
  int64_t new_sock=(int64_t)arg;
  HandleRequest(new_sock);
  return NULL;
}

void http_server_start(char* ip,short port)
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
  addr.sin_port=htons(port);

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
      continue;
    }
    printf("客户端连接：%s:%d\n",inet_ntoa(peer.sin_addr),peer.sin_port);
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
  signal(SIGCHLD,SIG_IGN);
  http_server_start(argv[1],atoi(argv[2]));
  return 0;
}
