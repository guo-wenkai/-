#include"thread.h"
#include<sys/wait.h>
#include<fcntl.h>
#define ARG_MAX 10
#define CMD_ERR "err arg"
#define PIPE_ERR "err pipe"
#define FORK_ERR "err fork"
#define OK "ok#"
#define FILE_ERR "err file"

void recv_file(int c,char cmd_buff[],char *filename)
{
	if(cmd_buff==NULL||filename==NULL)
	{
		printf("上传的文件为空\n");
		return;
	}
	send(c,cmd_buff,strlen(cmd_buff),0);


	char buff[64]={0};
	int num=recv(c,buff,63,0);
	if(num<0)
	{
		printf("cli close or err\n");
		return;
	}
	if(strncmp(buff,"err",3)==0)
	{
		printf("cli:%s\n",buff);
		return;
	}

	int filesize=0;
	sscanf(buff+3,"%d",&filesize);
	printf("上传文件：%s,文件大小：%d\n",filename,filesize);
	if(filesize<0)
	{
		send(c,"err",3,0);
		return;
	}

	int fd=open(filename,O_CREAT|O_WRONLY,0600);
	if(fd==-1)
	{
		printf("上传文件时创建文件失败!\n");
		send(c,"err",3,0);
		return;
	}
	send(c,"ok",2,0);
	char data[1024];
	int curr_size=0;

	while(1)
	{
		int n=recv(c,data,1024,0);
		if(n<0)
		{
			printf("上传文件失败\n");
			break;
		}
		write(fd,data,n);
		curr_size+=n;
		float f=curr_size*100.0/filesize;
		printf("当前下载:%.2f%%\r",f);

		fflush(stdout);
		if(curr_size>=filesize)
		{
			break;
		}
	}
	close(fd);
	printf("\n");
	printf("文件上传完成!\n");

    return;
}


void send_file(int c,char *filename)
{
	if(filename==NULL)
	{
		send(c,CMD_ERR,strlen(CMD_ERR),0);
		return;
	}
	int fd=open(filename,O_RDONLY);
	if(fd==-1)
	{
		printf("文件打开失败\n");
		send(c,FILE_ERR,strlen(FILE_ERR),0);
		return;
	}
	
	int filesize=lseek(fd,0,SEEK_END);
	lseek(fd,0,SEEK_SET);

	char buff_size[64]={0};
	sprintf(buff_size,"ok#%d",filesize);
	send(c,buff_size,strlen(buff_size),0);

	memset(buff_size,0,64);
	int n=recv(c,buff_size,63,0);
	if(n<=0)
	{
		return;
	}
	if(strcmp(buff_size,"err")==0)
	{
		return;
	}

	int downsize=0;
	sscanf(buff_size+3,"%d",&downsize);
	printf("该文件已经下载了%d个字节\n",downsize);
	downsize=lseek(fd,downsize,SEEK_SET);
	if(downsize==-1)
	{
		printf("文件打开失败\n");
		return;
	}
	char data[1024];
	int num=0;
	int sendnum=0;
	while((num=read(fd,data,1024))>0)
	{
		sendnum=send(c,data,num,0);
		if(sendnum==-1)
		{
			printf("发送数据失败\n");
			close(fd);
			printf("cloent close\n");
			return;
		}
	}
	close(fd);
	return;
}

char *get_cmd(char buff[],char *myargv[])
{
	if(buff==NULL||myargv==NULL)
	{
		return NULL;
	}

	char *s=NULL;
	char *p=NULL;

	int i=0;
	s=strtok_r(buff," ",&p);
	while(s!=NULL)
	{
		myargv[i++]=s;
		s=strtok_r(NULL," ",&p);
	}
	return myargv[0];
}

void * work_thread(void *arg)
{
	int c=(int)arg;
	while(1)
	{
		char buff[128]={0};
		int n=recv(c,buff,127,0);
		if(n<=0)
		{
			break;
		}
		char cmd_buff[128]={0};
		strcpy(cmd_buff,buff);
		printf("服务器端收到的命令：%s\n",cmd_buff);

		char *myargv[ARG_MAX]={0};
		char *cmd=get_cmd(buff,myargv);

		if(cmd==NULL)
		{
			send(c,CMD_ERR,strlen(CMD_ERR),0);
			continue;
		}

		if(strcmp(cmd,"get")==0)
		{
			send_file(c,myargv[1]);
		}
		else if(strcmp(cmd,"up")==0)
		{
			recv_file(c,cmd_buff,myargv[1]);
		}
		else
		{
			int pipefd[2];
			if(pipe(pipefd)==-1)
			{
				send(c,PIPE_ERR,strlen(PIPE_ERR),0);
				continue;
			}
			pid_t pid=fork();
			if(pid==-1)
			{
				send(c,FORK_ERR,strlen(FORK_ERR),0);
				continue;
			}
			if(pid==0)
			{
				close(pipefd[0]);
				dup2(pipefd[1],1);
				dup2(pipefd[1],2);

				execvp(cmd,myargv);
				perror("exec err");
				close(pipefd[1]);
				exit(0);
			}
			close(pipefd[1]);
			wait(NULL);
			char read_buff[1024]={OK};
			read(pipefd[0],read_buff+strlen(OK),1021);
			close(pipefd[0]);
			send(c,read_buff,strlen(read_buff),0);
		}
	}
	close(c);
	printf("client close\n");
}
void start_thread(int c)
{
	pthread_t id;
	pthread_create(&id,NULL,work_thread,(void *)c);
}

