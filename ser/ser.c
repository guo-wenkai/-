#include"socket.h"
#include"thread.h"

int main()
{
    int sockfd=socket_init();
    if(sockfd==-1)
    {
        printf("创建连接套接字失败\n");
        exit(0);
    }

    struct sockaddr_in caddr;
	//这里是客户端的套接字
    int len;
    while(1)
    {
        len=sizeof(caddr);
        int c=accept(sockfd,(struct sockaddr*)&caddr,&len);
        if(c<0)
        {
			printf("accept err\n");
            continue;
        }
        printf("accept c=%d\n",c);
        start_thread(c);
    }
}
