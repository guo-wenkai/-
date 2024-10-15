#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#define OK "ok#"
#define CMD_ERR "err cmd"
#define FILE_ERR "err file"
void  send_file(int c,char cmd_buff[],char *filename)
{
	if(cmd_buff==NULL||filename==NULL)
	{
		send(c,CMD_ERR,strlen(CMD_ERR),0);
		printf("文件名为空\n");
		return;
	}
    send(c,cmd_buff,strlen(cmd_buff),0);
	char buff[64]={0};
	recv(c,buff,strlen(buff),0);
	int fd=open(filename,O_RDONLY);
    if(fd==-1)
    {
       send(c,FILE_ERR,strlen(FILE_ERR),0);
	   printf("要上传的文件打开失败\n");
	   return;
    }
	int filesize=lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET);
	char buff_size[64]={0};
	sprintf(buff_size,"ok#%d",filesize);
	send(c,buff_size,strlen(buff_size),0);
	memset(buff_size,0,64);
	
	int n=recv(c,buff_size,0,64);
	if(n<0)
	{
		printf("err\n");
		return;
	}
	if(strcmp(buff_size,"err")==0)
	{
		printf("err\n");
		return;
	}

	char data[1024];
	int num=0;
	printf("开始发送文件:\n");
	while((num=read(fd,data,1024))>0)
	{
		send(c,data,num,0);
	}
	printf("文件发送完毕！\n");
	close(fd);
	return;
}

void recv_file(int c,char cmd_buff[],char* filename)
{
	if(cmd_buff==NULL||filename==NULL)
	{
		return;
	}
	send(c,cmd_buff,strlen(cmd_buff),0);

	char buff[64]={0};
	int num=recv(c,buff,63,0);
	if(num<=0)
	{
		printf("客户端recv err\n");
		return;
	}

	if(strncmp(buff,"err",3)==0)
	{
		printf("ser:%s\n",buff);
		return;
	}

	int filesize=0;
	sscanf(buff+3,"%d",&filesize);
	printf("文件：%s,大小：%d\n",filename,filesize);
	if(filesize<0)
	{
		send(c,"err",3,0);
		return;
	}
	int fd=open(filename,O_CREAT|O_WRONLY,0600);
	if(fd==-1)
	{
		printf("创建文件失败\n");
		send(c,"err",3,0);
		return;
	}

	int downsize=0;
	downsize=lseek(fd,0,SEEK_END);
	char buff_size[64]={0};
	sprintf(buff_size,"ok#%d",downsize);
	printf("该文件已经下载了%d个字节\n",downsize);
	send(c,buff_size,strlen(buff_size),0);
	char data[1024];
	int curr_size=0;
	while(1)
	{
		int n=recv(c,data,1024,0);
		if(n<=0)
		{
			printf("down file err\n");
			break;
		}
		write(fd,data,n);
		curr_size+=n;
		float f=(curr_size+downsize) * 100.0/filesize;
		printf("当前下载：%.2f%%\r",f);

		fflush(stdout);
		if((curr_size+downsize)>=filesize)
		{
			break;
		}
	}
	close(fd);
	printf("\n");
	printf("文件下载完成\n");
	return;
}


char *get_cmd(char buff[],char *myargv[])
{
	if(buff==NULL||myargv==NULL)
	{
		return NULL;
	}
	char *s=strtok(buff," ");
	int i=0;
	while(s!=NULL)
	{
		myargv[i++]=s;
		s=strtok(NULL," ");
	}
	return myargv[0];
}

int main()
{
	int sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd==-1)
	{
		printf("客户端创建套接字失败\n");
		exit(0);
	}
	struct sockaddr_in saddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family=AF_INET;
	saddr.sin_port=htons(6000);
	saddr.sin_addr.s_addr=inet_addr("127.0.0.1");

	int res=connect(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
	if(res==-1)
	{
		printf("connect err\n");
		close(sockfd);
		exit(0);
	}

	while(1)
	{
		printf("connect>>");
		fflush(stdout);
		char buff[128]={0};
		fgets(buff,128,stdin);
		buff[strlen(buff)-1]='\0';
		//复制一份buff
		char cmd_buff[128]={0};
		strcpy(cmd_buff,buff);
		
		char *myargv[10]={0};
		char *cmd=get_cmd(buff,myargv);
		if(cmd==NULL)
		{
			continue;
		}
		else if(strcmp(cmd,"exit")==0)
		{
			break;
		}
		else if(strcmp(cmd,"get")==0)
		{
			recv_file(sockfd,cmd_buff,myargv[1]);
		}
		else if(strcmp(cmd,"up")==0)
		{
			send_file(sockfd,cmd_buff,myargv[1]);
		}
		else
		{
			send(sockfd,cmd_buff,strlen(cmd_buff),0);
			char read_buff[1024]={0};
			int num=recv(sockfd,read_buff,1023,0);
			if(num<=0)
			{
				printf("ser close or err\n");
				break;
			}
			if(strncmp(read_buff,OK,strlen(OK))==0)
			{
				printf("%s\n",read_buff+strlen(OK));
			}
			else
			{
				printf("%s\n",read_buff);
			}
		}
	}
}



