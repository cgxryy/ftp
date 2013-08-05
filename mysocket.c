// =====================================================================================
// 
//       Filename:  mysocket.c
//
//    Description:  
//
//        Version:  1.0
//        Created:  2013年07月30日 19时43分40秒
//       Revision:  cgxr
//       Compiler:  gcc
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

#include "send_recv.c"

#define QUIT 	0
#define LS 	1
#define GET 	2
#define PUT 	3

void my_err(char *string,int line)
{
	fprintf(stderr,"line: %d ",line);
	perror(string);
	exit(1);
}

void 	my_err(char *string, int line);
int 	server(int client_fd);
void 	command_choice(int client_fd, unsigned int command, char *file);
void 	command_ls(int client_fd, unsigned int command, char *file);
void 	filename_deal(char *filename);
void 	command_get(int client_fd, unsigned int command, char *file);

int main(int argc, char *argv[])
{
	int 			client_fd;
	int 			client_len;
	int 			pid;
	int 			optval;
	int 			socket_fd;
	struct sockaddr_in 	ser_addr;
	struct sockaddr_in 	cli_addr;

	//初始化套接字
	memset(&ser_addr, 0, sizeof(struct sockaddr_in));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(4507);
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//创建
	socket_fd = socket(AF_INET,SOCK_STREAM,0);
	if (socket_fd < 0)
	{
		my_err("socket",__LINE__);
	}

	//设置套接字，使可以重新绑定端口
	optval = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&optval, sizeof(int)) < 0)
	my_err("setsockopt",__LINE__);
	
	// 套接字绑定
	if (bind(socket_fd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in)) < 0)
		my_err("bind",__LINE__);

	//监听
	if (listen(socket_fd, 128) < 0)
	      my_err("listen",__LINE__);

	//接受连接
	client_len = sizeof(struct sockaddr_in);
	while (1)
	{	
		client_fd = accept(socket_fd, (void*)&cli_addr,&client_len);	
		if (client_fd < 0)	      
		      my_err("accept",__LINE__);
	
		printf("有个客户端请求连接,ip:%s\n",inet_ntoa(cli_addr.sin_addr));
		//子进程处理连接请求
		if ((pid = fork()) == 0)
		{
			printf("已连接到客户端，正在子进程处理...\n");
			server(client_fd);//处理函数，处理接发数据
		}
	}

	close(socket_fd);
	return 0;

}

int server(int client_fd)
{
//	fd_set		choice_fd; 
//	struct timeval 	time_limit;
//	int 		ret = 1;//返回值判断函数是否出错
	int 		size;
	unsigned int 	cmd_num;
	char 		*file;
	
	while (1)
	{
/*		FD_ZERO(&choice_fd);
		FD_SET(client_fd, &choice_fd);
		time_limit.tv_sec = 300; //等待5分钟
		time_limit.tv_usec = 0;

		ret = select(client_fd+1, (fd_set*)&choice_fd, NULL, NULL, &time_limit);
		if (ret == 0)
		{
			printf("已超时，返回重试...\n");
			return 0;
		}
		if (ret == -1)//所有描述集清0，重新返回判断
			continue;
*/		
		size = recv(client_fd, &cmd_num, sizeof(cmd_num), 0);
		
		cmd_num = ntohl(cmd_num);
		printf("接受到数据%d字节",size);
		
		if (size < 0)
			my_err("recv",__LINE__);
		if (size == 0)
		{
			printf("客户端断开连接...\n");
			return -1;
		}
		
		file = malloc(256);
		memset(file, 0, 256);
		
		size = recv(client_fd, file, 256, 0);
		printf("接受到文件名数据%d字节",size);
		printf("%s", (char*)file);
		printf("序号为%u\n", cmd_num);
		
		free(file);
		command_choice(client_fd, cmd_num, file);

	}
}

void command_choice(int client_fd, unsigned int command, char *file)
{
	
	printf("进入command_choice()...\n");
	printf("未处理时文件名%s\n",file);
	filename_deal(file);
	switch (command)
	{
		case QUIT:	      
			printf("客户端主动断开连接...\n"); 
			close(client_fd);	      
			break;
		case LS:	
			command_ls(client_fd, command, file);
			break;
		case GET:
			command_get(client_fd, command, file);
			break;
		case PUT:	     
			break;
	}
}

void command_ls(int client_fd, unsigned int command, char *file)
{
	DIR 		*dir;
	struct dirent 	*into;
	char 		filename[4096];
	int 		i = 0;
	int 		ret;
	char 		workspace[20] = "/home/chang/cgxr/my";

	printf("进入command_ls()...\n");
	memset((void*)&filename, '\0', (size_t)sizeof(filename));

	dir = opendir(file);
	if (dir == NULL)
	      my_err("opendir",__LINE__);

	while ((into = readdir(dir)) != NULL)
	{
		strcat(filename, into->d_name);
		strcat(filename, "\t");
		i++;
	}
	
	printf("开始发送...\n");
	ret = send(client_fd, (char *)&filename, 4096, 0);
	if (ret == -1)
	      my_err("my_send", __LINE__);
	if (ret < sizeof(filename))
	      printf("数据发送不完整...错误...");
	
	chdir(workspace);
	closedir(dir);
}

void filename_deal(char *file)
{
	char 	temp[256];
	char 	*last = file;

	printf("进入filename_deal()...\n");
	
	//在文件夹名前加上/home/chang/changgong
	strcpy(temp, file);
	strcpy(file,"/home/chang/changgong");
	strcat(file, temp);
	
	//如果文件夹最后有/，则去掉
	while (*last != '\0')
	{
		last++;
	}
	last--;
	if (*last == '/')
	{
		*last = '\0';
	}
	printf("修改完此时文件名为%s", file);
}

void command_get(int client_fd, unsigned int command, char *file)
{
	char 	*buf;//用户自定义缓存区
	int 	ret;//每次实际发送大小
	char 	file_fd;//打开的文件描述符

	buf = malloc(4096);
	
	file = open();

	while (ret < 4096)
}
