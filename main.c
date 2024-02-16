#include "head.h"
#include "function.h"

#define SERVER_PORT 80

int main()
{
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET,SOCK_STREAM,0);

    bzero(&server_addr,sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    bind(sock,(struct sockaddr*)&server_addr,sizeof(server_addr));

    listen(sock,128);

    printf("等待客户端连接\n");

    int done = 1;

    while(done)
    {
        struct sockaddr_in client;
        int client_sock,len,i;
        char client_ip[64];
        char buf[256];
        pthread_t id;
        int* pclient_sock = NULL;

        socklen_t client_addr_len;
        client_addr_len = sizeof(client);
        client_sock = accept(sock,(struct sockaddr*)&client,&client_addr_len);

        printf("client ip: %s\t port: %d\n",
        inet_ntop(AF_INET,&client.sin_addr.s_addr,client_ip,sizeof(client_ip)),
        ntohs(client.sin_port));

        //do_http_request(client_sock);

        //启动线程处理 http 请求
        pclient_sock = (int*)malloc(sizeof(int));
        *pclient_sock = client_sock;
        pthread_create(&id,NULL,(void*)do_http_request,(void*)pclient_sock);

        //close(client_sock);
    }

    //close(sock);

    return 0;
}
