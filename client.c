// =====================================================================================
// 
//       Filename:  client.c
//
//    Description:  
//
//        Version:  1.0
//        Created:  2013年08月02日 15时41分36秒
//       Revision:  none
//       Compiler:  g++
//
//         Author:  hanyaorong, cgxryy@gmail.com
//        Company:  Class 1201 of Computer Network Technology
// 
// =====================================================================================

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#include "send_recv.c"

#define QUIT 	0
#define LS 	1
#define GET 	2
#define PUT 	3
#define HELP    9

void 	my_err(char *string, int line);
void 	information();
int 	client(int sock_fd);
int 	deal_server(int sock_fd);
void 	command_ls(int sock_fd, char *buf, char *file);
void 	deal_ls(char *buf, char filename[256][256]);
void 	show_ls(char filename[][256]);
void 	command_get(int sock_fd, char *buf, char *file);

void information()
{
	printf("./XXX IP地址 端口\n\t./client 192.168.100.1 4507 (例子)\n");
}

void my_err(char *string,int line)
{
	fprintf(stderr, "line :%d ", line);
	perror(string);
	exit(1);
}

int main(int argc, char *argv[])
{
	struct sockaddr_in 	sock;
	int 			sock_fd;
	unsigned int 		port;
	int 			ret;

	if ( argc != 3)
	{
		information();
		return -1;
	}
	
	port = atoi(argv[2]);
	if (port < 0 || port > 65535)
		printf("并不存在此端口\n");

	//初始化地址结构
	memset(&sock, 0, sizeof(struct sockaddr_in));
	sock.sin_family = AF_INET;
	sock.sin_port = htons(port);

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0)
	      my_err("socket",__LINE__);
	ret = inet_aton(argv[1], &sock.sin_addr);
	if (ret <= 0)
	{
		printf("IP不可用\n");
		information();
		return -1;
	}
	
	if (connect(sock_fd, (struct sockaddr*)&sock, sizeof(struct sockaddr)) < 0)
	      my_err("connect", __LINE__);

	client(sock_fd);

	return 0;
}

int 	client(int sock_fd)
{
			
	deal_server(sock_fd);
	return 0;
}

int 	deal_server(int sock_fd)
{
	int 		ret;
	char 		file[256];
	char  		buf[4096];
	char 		command[256];
	unsigned int 	cmd_num;
	
	while (1)
	{
		setbuf(stdin, NULL);
		printf("输入命令\n");
		scanf("%[^\n]",command);

		if (strncmp(command,"quit", 4) == 0)
		      cmd_num = QUIT;
		else if (strncmp(command,"ls", 2) == 0)
		      cmd_num = LS;
		else if (strncmp(command,"get", 3) == 0)
		      cmd_num = GET;
		else if (strncmp(command,"put", 3) == 0)
		      cmd_num = PUT;
		else if (strncmp(command, "help", 4) == 0)
		      cmd_num = HELP;

		printf("发送命令为%s,编号%u", command, cmd_num);
	
		cmd_num = htonl(cmd_num);
		ret = send(sock_fd, &cmd_num,sizeof(cmd_num), 0);
		if (ret < sizeof(cmd_num))
		{
			printf("发送时丢失数据包...错误...\n");
			return -1;
		}
		printf("已发送%d字节数\n", ret);
		
		cmd_num = ntohl(cmd_num);
		//决定发送什么内容
		switch (cmd_num)
		{	
			case QUIT:
				return 0;
			case LS:
				setbuf(stdin, NULL);
				printf("目录名,首次则输/ \n");
				scanf("%[^\n]", file);
				break;
			case GET:
				setbuf(stdin, NULL);
				printf("请输入要接收的文件名:\n");
				scanf("%[^\n]", file);
				break;
			case PUT:
				break;
			case HELP:
				break;
		}
		if (cmd_num != HELP)
		{	
			ret = send(sock_fd, file, 256, 0);
			if (ret < 256)
			{
				printf("发送时丢失数据包...错误...\n");
				return -1;
			}
			printf("已发送%d字节数\n", ret);
		}
		else
		      printf("命令列表:\n\tquit\tls\tget\tput\thelp\n");
	
		//根据发送内容不同，接受下一个包方式不同
		switch (cmd_num)
		{	
			case QUIT:
				break;
			case LS:
				command_ls(sock_fd, buf, file);
				break;
			case GET:
				command_get(sock_fd, buf, file);
				break;
			case PUT:
				break;
		}

	}
}

void command_ls(int sock_fd, char *buf, char *file)
{
	char 	filename[256][256] = {'\0'};
	int 	ret;

	ret = recv(sock_fd, (void*)buf, 4096, 0);
	printf("已经接受到%d字节\n", ret);
	if (ret < 0)
	     	my_err("recv",__LINE__);
	if (ret == 0)
		printf("异常断开连接...\n");
				
	deal_ls(buf,filename);
	show_ls(filename);
}

void deal_ls(char *buf, char filename[][256])
{
	char 	*move = buf;
	int 	i = 0, j = 0;

	while (*move != '\0')
	{
		if (*move == '\t')
		{
			filename[i][j] = '\0';
			i++;
			j = 0;
			move++;
		}
		filename[i][j] = *move;
		move++;
		j++;
	}
}

void show_ls(char filename[][256])
{
	int  	maxlen = 0;
	int  	nowlen = 0;
	int  	i = 0,linemax = 0,j;

					
	maxlen = strlen(filename[i]);
						
	while ( filename[i][0] != '\0')
	{
	
		if (maxlen < strlen(filename[i+1]))		      
		      maxlen = strlen(filename[i+1]);
		i++;
	}
							
	i = 0;
	linemax = maxlen;
	while ( filename[i][0] != '\0')
	{
		nowlen = strlen(filename[i]);
		if ((linemax + 2 ) > 120)
		{
			printf("\n");
			printf("%s",filename[i]);
			for ( j = 0; j < maxlen - nowlen + 2; j++)
			     printf(" ");
			linemax = maxlen;
			i++;
		}
		else
		{      
			printf("%s",filename[i]);
			for ( j = 0; j < maxlen - nowlen + 2; j++)
				   printf(" ");
			linemax += (maxlen + 2);
			i++;
		}
	}
	printf("\n");
}

void command_get(int sock_fd, char *buf, char *file)
{
	int 	file_fd;//新建的文件的文件描述符
	char 	*now;//文件指针现在位置
	int 	ret = 4096;//每次实际接收大小
	unsigned long sum = 0;//接收总大小，测试用
	char 	user[256];//存放user名
	char  	path[256];
	char 	former_path[256];
	char 	*receive = former_path;
	DIR 	*dir;
	int 	recv_ret = 1;
	int 	write_ret = 1;
	int 	i = 0;	
	//最后需恢复成原来工作路径
/*      receive = getcwd(receive, 256);
	if (receive == NULL)
	      my_err("getcwd", __LINE__);
	//把接收的文件送到当前用户的receive_ftp目录下
	receive = getlogin();
	strcpy(user, receive);
	strcpy(path, "/home/");
	strcat(path, user);
	mkdir("receive_ftp",0644);
	strcat(path, "/receive_ftp");

	chdir_ret = chdir(path);
	printf("当前目录为%s\n", path);
	
	assert(chdir_ret == 0);
	*/
	//创建文件，写入
	file_fd = open(file, O_CREAT|O_EXCL|O_RDWR, 0644);
	if (file_fd == -1)
	      my_err("malloc",__LINE__);

	while (1)
	{
		memset(buf, 0, 4096);
		recv_ret = recv(sock_fd, buf, 4096, 0);
		printf("接收到%d字节，准备写入...\n", recv_ret);
		if (recv_ret == -1)
		      my_err("recv",__LINE__);
		//需要移动指针，否则始终写入前4096字节
		write_ret = write(file_fd, buf, recv_ret-1);
		printf("写入文件%d字节....\n", write_ret);
		if (write_ret == -1)
		      my_err("write",__LINE__);
		i++;
	}
	printf("\n\t共写入%d次", i);

	close(file_fd);
	printf("恢复到之前目录%s\n",former_path);
	chdir(former_path);
}
