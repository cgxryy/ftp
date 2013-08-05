int my_send(int sock_fd, char *buf, int size, int flag)
{
	int ret;
	int leftlen = size;

	while (leftlen > 0)
	{
		ret = send(sock_fd, buf, leftlen, flag);
		if (ret == -1 )
		      return -1;
		printf("发送了%d字节, 剩余%d字节\n", ret, leftlen);
		if (leftlen > 0)
		{
			buf += ret;
			leftlen -= ret;
		}
	}
	return size;
}

int my_recv(int sock_fd, char *buf, int size, int flag)
{
	int ret; 
	int leftlen = size;

	while (leftlen > 0)
	{
		ret = recv(sock_fd, buf, leftlen, flag);
		if (ret == -1 )
		      return -1;
		printf("接收到%d字节,剩余%d字节\n", ret, leftlen);
		if (leftlen > 0)
		{
			buf += ret;
			leftlen -= ret;
		}
	}
	return size;
}
