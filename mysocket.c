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
#include <sys/stat.h>
#include <fcntl.h>

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
		if (size < 256)
		      my_err("recv", __LINE__);
		printf("接受到文件名数据%d字节",size);
		printf("%s", (char*)file);
		printf("序号为%u\n", cmd_num);
		
		command_choice(client_fd, cmd_num, file);

		free(file);
	}
}

void command_choice(int client_fd, unsigned int command, char *file)
{
	
	printf("进入command_choice()...\n");
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

	printf("未处理时文件名%s\n",file);
	filename_deal(file);

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
	char 	*now;//现在文件指针所在处
	int 	send_ret = 4096;
	int 	read_ret = 4096;//每次实际发送大小,为了循环第一次判断初值为大于0的值
	int  	file_fd;//打开的文件描述符
	unsigned long sum = 0;//测试时统计发送总大小	
	char 	former_path[256];
	char 	*receive = former_path;
	int 	i = 0;

	receive = getcwd(receive, 256);
	if (receive == NULL)
	      my_err("getcwd", __LINE__);
	//转换到文件所在目录
	chdir("/home/chang/changgong");
	printf("已经转到/home/chang/changgong目录\n");

	//开始读出发送
	buf = malloc(sizeof(int)+ 4096);
	file_fd = open(file, O_RDWR, 0644);
	if (file_fd == -1)
	      my_err("open", __LINE__);

	//全部发送完时ret为0
	while (1)
	{
		//buf所指空间第一个字符为标志位，表示是否传完
		memset(buf, 0, 4096);
		read_ret = read(file_fd, buf, 4096);
		printf("读出%d字节信息，准备发送...\n", read_ret);
		if (read_ret == -1)
		      my_err("read", __LINE__);
		//read_ret实际读出值，也是要发送出的大小
		send_ret = send(client_fd, buf, read_ret, 0);
		printf("发送%d字节信息...\n", send_ret);
		if (send_ret == -1)
		      my_err("send", __LINE__);	
		i++;
	}
	printf("\n\t共发送%d次\n", i);
	
	close(file_fd);
	free(buf);
	printf("恢复到之前目录%s\n",former_path);
	chdir(former_path);
}
