#include<stdio.h>
#include<stdlib.h>
#include<mysql/mysql.h>

int main()
{
  //使用MySQL API完成查找工作；
  //查找相关数据库的所有数据；
  //正常情况下，第一步应该先根据CGI的规则；
  //获取到用户定义的相关参数，但是由于此处是查找表的数据，忽略用户输入；
  //1.初始化MySQL的连接句柄；
  MYSQL* connect_fd=mysql_init(NULL);
  //2.建立连接
  if(mysql_real_connect(connect_fd,"127.0.0.1","root","123456","student",3306,NULL,0)==NULL)
  {
    fprintf(stderr,"mysql connect error\n");
    return 1;
  }
  fprintf(stderr,"mysql connect success\n");
  //MYSQL_real_connect函数参数：
  //a.MySQL连接句柄
  //b.MySQL服务器的主机地址；
  //c.用户名
  //d.密码
  //e.要访问的数据库名；
  //f.端口号
  //g.Unix套接字
  //h.标志位
  //3.构造SQL语句；
  const char* sql="select * from student";

  //4.把SQL语句发送到服务器；
  int ret= mysql_query(connect_fd,sql);
  if(ret<0)
  {
    fprintf(stderr,"mysql_query error\n");
    return 1;
  }
  //5.遍历结果集合（表结构）；
  //先按行遍历，每一次取出一行，再依次遍历出来；
  MYSQL_RES* result=mysql_store_result(connect_fd);
  if(result==NULL)
  {
    fprintf(stderr,"mysql_store_result error\n");
    return 1;
  }
  printf("<html>");
  //a.先获取到有几行，以及几列；
  int rows=mysql_num_rows(result);
  int fields=mysql_num_fields(result);
  //b.把表头中的这几个字段都获取到，调用一次的mysql_fetch_field(result);
  MYSQL_FIELD* f=mysql_fetch_field(result);
  while(f!=NULL)
  {
    //此处的name就表示这列的具体的列名
    printf("%s\t",f->name);
    f=mysql_fetch_field(result);
  }
  printf("<br>");
  //c.把表内容获取出来
  int i=0;
  for(;i<rows;++i)
  {
    MYSQL_ROW row=mysql_fetch_row(result);
    int j=0;
    for(;j<fields;++j)
    {
      printf("%s\t",row[j]);
    }
    printf("<br>");
  }
  //6.断开连接；
  mysql_close(connect_fd);
  return 0;
}
