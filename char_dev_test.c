/*************************************************************************
  > File Name: char_dev_test.c
  > Author: WangDengtao
  > Mail: 1799055460@qq.com 
  > Created Time: 2023年03月16日 星期四 16时50分29秒
 ************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
	int fd;
	char buf[1024];
	int len;
	/* 1. 判断参数 */
	if (argc < 2)
	{
		printf("Usage: %s -w <string> /dev/??\n", argv[0]);
		printf(" %s -r\n", argv[0]);
		return -1;
	}
	/* 2. 打开文件 */
	if(argc == 4)// ./char_dev_test -w  hellow  /dev/chardev  // #define DEV_NAME  "chardev"       /*宏定义设备的名字*/
	{
		fd = open(argv[3], O_RDWR);
	}
	if(argc == 3)// ./char_dev_test -r /dev/chardev
	{
		fd = open(argv[2], O_RDWR);
	}
	if (fd == -1)//打开字符设备失败
	{
		printf("can not open file %s\n", argv[3]);
		return -1;
	}
	/* 3. 写文件或读文件 */
	if ((0 == strcmp(argv[1], "-w")) && (argc == 4))//四个参数，其中第一个参数为 -w时，写操作
	{
		len = strlen(argv[2]) + 1;
		len = len < 1024 ? len : 1024;
		printf("Write to %s success!\n", argv[3]);
		printf("write len: %d\n", len);
		write(fd, argv[2], len);
	}
	else if((0 == strcmp(argv[1], "-r")) && (argc == 3))
	{
		memset(buf, 0, sizeof(buf));
		len = read(fd, buf, 1024);
		printf("Read from %s success!\n", argv[2]);
		printf("read len: %ld\n", strlen(buf)+1);
		buf[1023] = '\0';
		printf("char_dev_test read : %s\n", buf);
	}
	else
	{
		printf("Usage: %s -w <string> /dev/??\n", argv[0]);
		printf(" %s -r\n", argv[0]);
		return -1;
	}
	close(fd);
	return 0;
}
