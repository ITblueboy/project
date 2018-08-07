#include<stdio.h>
#include<mysql/mysql.h>
#include<stdlib.h>
#include"cgi_base.h"

int main()
{
  //1.获取query_string并解析
  char buf[1024*4]={0};
  if(GetQueryString[buf]<0)
  {
    fprintf(stderr,"get querystring error\n");
    return 1;
  }
  int id=0;
  char name[100]={0};
  sscanf(buf,"id=%d&name=%s",&id,name);
  //2.初始化mysql连接句柄
  MYSQL* connect_fd=mysql_init(NULL);
  //3.建立连接
  if(mysql_real_connect(connect_fd,"127.0.0.1"))
  //4.构造SQL语句
  //5.发送SQL语句
  //6.断开连接
}
