#include"socket.h"

#define IPS "ipstr="
#define PORT "port="
#define LISMAX "lismax="

struct sock_info
{
    char ips[32];
    short port;
    short lismax;
};

int read_conf(struct sock_info *pa)
{
    if(pa==NULL)
    {
        return -1;
    }

    FILE *fp=fopen("my.conf","r");
    if(fp==NULL)
    {
        printf("my.conf不存在\n");
        return -1;
    }
    
    int index=0;
    char buff[128]={0};
    while(fgets(buff,128,fp)!=NULL)
    {
        index++;
        if(strncmp(buff,"#",1)==0)
        {
            continue;
        }
        if(strncmp(buff,"\n",1)==0)
        {
            continue;
        }
        
        buff[strlen(buff)-1]='\0';
        if(strncmp(buff,IPS,strlen(IPS))==0)
        {
            strcpy(pa->ips,buff+strlen(IPS));
        }
        else if(strncmp(buff,PORT,strlen(PORT))==0)
        {
            pa->port=atoi(buff+strlen(PORT));
        }
        else if(strncmp(buff,LISMAX,strlen(LISMAX))==0)
        {
            pa->lismax=atoi(buff+strlen(LISMAX));
        }
        else
        {
            printf("未识别的配置项在第%d行:%s\n",index,buff);
        }
    }
    fclose(fp);
}

int socket_init()
{
    struct sock_info a;
    if(read_conf(&a)==-1)
    {
        printf("read conf error!\n");
        return -1;
    }
    printf("ip:%s\n",a.ips);
    printf("port:%d\n",a.port);
    printf("lismax:%d\n",a.lismax);

    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd==-1)
    {
		printf("创建套接字失败\n");
        return sockfd;
    }

    struct sockaddr_in saddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(a.port);

    int res=bind(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
    if(res==-1)
    {
		printf("绑定失败\n");
        return -1;
    }

    res=listen(sockfd,a.lismax);
    if(res==-1)
    {
		printf("监听失败\n");
        return -1;
    }
    return sockfd;
}
