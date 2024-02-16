#include "function.h"

static int debug = 1;

void* do_http_request(void* pclient_sock)
{
    /*读取客户端发送的 http 请求*/
    int len = 0;
    char buf[256];
    char method[64];
    char url[256];
    char path[256];
    int client_sock = *(int*)pclient_sock;
    struct stat st;

    //1.读取请求行
        len = get_line(client_sock,buf,sizeof(buf));
        //printf("read line : %s\n",buf);

        if(len > 0)//读到请求行
        {
            int i = 0,j = 0;

            while(!isspace(buf[j])&&i<sizeof(method)-1)
            {
                method[i] = buf[j];
                i++;j++;
            }

            method[i] = '\0';
            if(debug) printf("request method: %s\n",method);

            if(strncasecmp(method,"GET",i)==0)//处理GET请求
            {
                if(debug) printf("method = GET\n");

                //获取URL
                while(isspace(buf[j++]));//跳过白空格
                i = 0;
                while(!isspace(buf[j])&&i<sizeof(url)-1)
                {
                    url[i] = buf[j];
                    i++;j++;
                } 

                url[i] = '\0';

                if(debug) printf("URL: %s\n",url);
                //继续读取 http 头部
                do
                {
                    len = get_line(client_sock,buf,sizeof(buf));
                    if(debug) printf("read: %s\n",buf);
                } while(len>0);

                /*定义服务器本地的 html 文件*/

                //处理URL中的 ？ 
                {
                    char *pos = strchr(url,'?');
                    if(pos)
                    {
                        *pos = '\0';
                        printf("read url: %s\n",url);
                    }
                }
                sprintf(path,"./html_docs/%s",url);
                if(debug) printf("path: %s\n",path);

                //执行 http 响应
                //判断文件是否存在 如果存在就响应 200 OK 同时发送相应的 html 文件 如果不存在 响应 404 NOT FOUND
                if(-1 == stat(path,&st))//文件不存或出错
                {
                    fprintf(stderr,"stat failed: %s\n",strerror(errno));
                    not_found(client_sock);
                }
                else//文件存在
                {
                    if(S_ISDIR(st.st_mode))
                    {
                        strcat(path,"/index.html");
                    }
                    do_http_response(client_sock,path);
                }
            }
            else//非GET请求 读取 http 头部 并响应客户端501 Method Not Implemented
            {
                fprintf(stderr,"warning! other request [%s]\n",method);
                do
                {
                    len = get_line(client_sock,buf,sizeof(buf));
                    if(debug) printf("read: %s\n",buf);
                } while(len>0);

                unimplemented(client_sock);//501
            }
        }
        else//请求格式有问题 出错
        {
            bad_request(client_sock);//400
        }
        close(client_sock);
        if(pclient_sock) free(pclient_sock);//释放动态分配的内存

    return NULL;
}

void do_http_response_t(int client_sock)//硬编码方式
{
    const char *main_header = "HTTP/1.0 200 OK\r\nServer: Martin Server\r\nContent-Type: text/html\r\nConnection: Close\r\n";


    const char * welcome_content = "\
<html lang=\"zh-CN\">\n\
<head>\n\
<meta content=\"text/html; charset=utf-8\" http-equiv=\"Content-Type\">\n\
<title>This is a test</title>\n\
</head>\n\
<body>\n\
<div align=center height=\"500px\" >\n\
<br/><br/><br/>\n\
<h2>大家好,欢迎来到奇牛学院VIP 试听课！</h2><br/><br/>\n\
<form action=\"commit\" method=\"post\">\n\
尊姓大名: <input type=\"text\" name=\"name\" />\n\
<br/>芳龄几何: <input type=\"password\" name=\"age\" />\n\
<br/><br/><br/><input type=\"submit\" value=\"提交\" />\n\
<input type=\"reset\" value=\"重置\" />\n\
</form>\n\
</div>\n\
</body>\n\
</html>";
    //1.发送 main_header
    int len = write(client_sock,main_header,strlen(main_header));
    if(debug) fprintf(stdout,"do_http_response...\n");
    if(debug) fprintf(stdout,"write[%d]: %s",len,main_header);

    //2.生成 Content-Length 行并发送
    char send_buf[64];
    int wc_len = strlen(welcome_content);
    len = snprintf(send_buf,64,"Content-Length: %d\r\n\r\n",wc_len);
    len = write(client_sock,send_buf,len);
    if(debug) fprintf(stdout,"write[%d]: %s",len,send_buf);

    //3.发送 html 文件内容
    len = write(client_sock,welcome_content,wc_len);
    if(debug) fprintf(stdout,"write[%d]: %s",wc_len,welcome_content);
}

void do_http_response(int client_sock,const char* path)
{
    int ret = 0;
    FILE* resource = NULL;

    resource = fopen(path,"r");

    if(resource == NULL)
    {
        not_found(client_sock);
        return;
    }

    //1.发送 http 头部
    ret = headers(client_sock,resource);

    //2.发送 http body
    if(!ret)
    {
        cat(client_sock,resource);
    }

    fclose(resource);

}

//http 头部
int headers(int client_sock,FILE* resource)
{
    struct stat st;
    int file_id = 0;
    char tmp[64];
    char buf[1024] = {0};
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
	strcat(buf, "Server: Martin Server\r\n");
	strcat(buf, "Content-Type: text/html\r\n");
	strcat(buf, "Connection: Close\r\n");

    file_id = fileno(resource);

    if(-1 == fstat(file_id,&st))
    {
        inner_error(client_sock);
        return -1;
    }

    snprintf(tmp,64,"Content-Length: %ld\r\n\r\n",st.st_size);
    strcat(buf,tmp);

    if(debug) fprintf(stdout,"header: %s\n",buf);

    if(send(client_sock,buf,strlen(buf),0)<0)
    {
        fprintf(stderr,"send failed data: %s, reason %s\n",buf,strerror(errno));
        return -1;
    }

    return 0;
}

//将 html 文件的内容按行读取并发送给客户端
void cat(int client_sock,FILE* resource)
{
    char buf[1024];

    fgets(buf,sizeof(buf),resource);

    while(!feof(resource))//如果没有到达文件尾部
    {
        int len = write(client_sock,buf,strlen(buf));

        if(len<0)//发送 body 的过程中出现问题
        {
            fprintf(stderr,"send body error reason: %s\n",strerror(errno));
            break;
        }

        if(debug) fprintf(stdout,"%s",buf);

        fgets(buf,sizeof(buf),resource);
    }
}

//返回值：-1 表示读取出错， 0 表示读取到空行， 大于0表示读取成功
int get_line(int sock,char* buf,int size)
{
    int count = 0;
    char ch = '\0';
    char len = 0;

    while((count<size-1)&&(ch!='\n'))
    {
        len = read(sock,&ch,1);

        if(1==len)
        {
            if('\r'==ch) continue;

            if('\n'==ch)
            {
                buf[count] = '\0';
                break;
            }

            buf[count] = ch;
            count++;
        }else if(-1==len)//read返回-1 代表读取出错
        {
            perror("read failed!\n");
            count = -1;
            break;
        }else if(0==len)//read返回0 代表客户端关闭 socket 连接
        {
            fprintf(stderr,"client close!\n");
            count = -1;
            break;
        }
    }

    if(count >= 0) buf[count] = '\0';

    return count;
}

void not_found(int client_sock)//404
{
    const char * reply = "HTTP/1.0 404 NOT FOUND\r\n\
Content-Type: text/html\r\n\
\r\n\
<HTML lang=\"zh-CN\">\r\n\
<meta content=\"text/html; charset=utf-8\" http-equiv=\"Content-Type\">\r\n\
<HEAD>\r\n\
<TITLE>NOT FOUND</TITLE>\r\n\
</HEAD>\r\n\
<BODY>\r\n\
	<P>文件不存在！\r\n\
    <P>The server could not fulfill your request because the resource specified is unavailable or nonexistent.\r\n\
</BODY>\r\n\
</HTML>";

	int len = write(client_sock, reply, strlen(reply));
	if(debug) fprintf(stdout, reply);
	
	if(len <=0)
    {
		fprintf(stderr, "send reply failed. reason: %s\n", strerror(errno));
	}
}

void unimplemented(int client_sock)//501
{
    const char * reply = "HTTP/1.0 501 Method Not Implemented\r\n\
Content-Type: text/html\r\n\
\r\n\
<HTML>\r\n\
<HEAD>\r\n\
<TITLE>Method Not Implemented</TITLE>\r\n\
</HEAD>\r\n\
<BODY>\r\n\
    <P>HTTP request method not supported.\r\n\
</BODY>\r\n\
</HTML>";

    int len = write(client_sock, reply, strlen(reply));
	if(debug) fprintf(stdout, reply);
	
	if(len <=0)
    {
		fprintf(stderr, "send reply failed. reason: %s\n", strerror(errno));
	}
}

void bad_request(int client_sock)//400
{
   const char * reply = "HTTP/1.0 400 BAD REQUEST\r\n\
Content-Type: text/html\r\n\
\r\n\
<HTML>\r\n\
<HEAD>\r\n\
<TITLE>BAD REQUEST</TITLE>\r\n\
</HEAD>\r\n\
<BODY>\r\n\
    <P>Your browser sent a bad request！\r\n\
</BODY>\r\n\
</HTML>"; 

    int len = write(client_sock, reply, strlen(reply));
	if(debug) fprintf(stdout, reply);
	
	if(len <=0)
    {
		fprintf(stderr, "send reply failed. reason: %s\n", strerror(errno));
	}
}

void inner_error(int client_sock)
{
    const char * reply = "HTTP/1.0 500 Internal Sever Error\r\n\
Content-Type: text/html\r\n\
\r\n\
<HTML lang=\"zh-CN\">\r\n\
<meta content=\"text/html; charset=utf-8\" http-equiv=\"Content-Type\">\r\n\
<HEAD>\r\n\
<TITLE>Inner Error</TITLE>\r\n\
</HEAD>\r\n\
<BODY>\r\n\
    <P>服务器内部出错.\r\n\
</BODY>\r\n\
</HTML>";

	int len = write(client_sock, reply, strlen(reply));
	if(debug) fprintf(stdout, reply);
	
	if(len <=0)
    {
		fprintf(stderr, "send reply failed. reason: %s\n", strerror(errno));
	}
}