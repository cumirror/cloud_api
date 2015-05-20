#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>

#define REMOTE_SERVER "www.baidu.com"
#define REMOTE_PORT 80
#define SERV_ADDR "127.0.0.1"
#define SERV_PORT  5555
#define BUF_SIZE (1024*1024)
#define FILE_BUF_SIZE (2*1024*1024)

const char *buf =	"POST /api/cloud2 HTTP/1.1\r\n"
				  	"Accept-Encoding: identity\r\n"
				  	"Host: www.baidu.com\r\n"
				  	"Content-Type: application/x-www-form-urlencoded\r\n"
				  	"Connection: close\r\n"
				  	"User-Agent: IT\r\n";
				  	//"Content-Length: 300\r\n"
					//"\r\n";

int md5_test(int connfd)
{
	int len;
	char msg[BUF_SIZE] = {0};
	char data[BUF_SIZE] = {0};


	strcat(data, "{"); 
	strcat(data, "\"ver\": \"3.5\",");
	strcat(data, "\"uid\": \"ffffffffffffffffffffffffffffffff\","); 
	strcat(data, "\"key\": \"testkey\","); 
	strcat(data, "\"type\": \"fileinfo\","); 
	strcat(data, "\"ip\": \"127.0.0.1\","); 
	strcat(data, "\"pid\": \"internal\","); 
	strcat(data, "\"data\": { \"data\":[{"); 
	//strcat(data, "\"crc\": \"\","); 
	//strcat(data, "\"alen\": \"9921\","); 
	strcat(data, "\"infotype\": \"fileinfov1\","); 
	strcat(data, "\"md5\": \"A978A548AF424B2DA49D2AC7477FE9A2\","); 
	strcat(data, "\"path\": \"test.zip\""); 
	strcat(data, "}],"); 
	strcat(data, "\"type\": \"file\""); 
	strcat(data, "}"); 
	strcat(data, "}"); 

	snprintf(msg, BUF_SIZE, "%s%s%d%s%s", buf, "Content-Length: ", strlen(data), "\r\n\r\n", data);
	printf("%s\n", msg);

	len = write(connfd, msg, strlen(msg));
	if (len < 0) {
		printf("发送失败\n");
	} else {
		printf("发送成功\n");
	}

	len = recv(connfd, msg, BUF_SIZE, MSG_WAITALL);
	if (len > 0)
		write(1, msg, len);
	else
		printf("发送成功\n");

	return 0;
}

#include "b64.h"
int file_test(int connfd, char *file)
{
	int fd;
	char *fb, *fb64, *tmp, *data;
	int n, len;
	unsigned int crc;

	/* 不要让栈太大 */
	fb = (char*)calloc(1, FILE_BUF_SIZE);
	tmp = (char*)calloc(1, FILE_BUF_SIZE);
	data = (char*)calloc(1, FILE_BUF_SIZE);

	/* 获取数据buf */
	fd = open(file, O_RDONLY);
	n = read(fd, fb, FILE_BUF_SIZE);
	if (n <= 0) {
		printf("read from file %s fd %d error\n", file, fd);
		return -1;
	}

	/* 计算文件的crc */
    crc = crc32_file(fb, n);
	printf("file %s, crc %x\n", file, crc);

	/* 进行base64加密 */
	fb64 = b64_encode(fb, n);

	/* 封装 */
	strcat(data, "{"); 
	strcat(data, "\"ver\": \"3.5\",");
	strcat(data, "\"uid\": \"ffffffffffffffffffffffffffffffff\","); 
	strcat(data, "\"key\": \"testkey\","); 
	strcat(data, "\"type\": \"test\","); 
	strcat(data, "\"ip\": \"127.0.0.1\","); 
	strcat(data, "\"pid\": \"internal\","); 

	strcat(data, "\"data\": { ");//\"data\":[{"); 
	strcat(data, "\"all\": \"1\","); 
	strcat(data, "\"datatype\": \"\","); 
	strcat(data, "\"filetype\": \"test\","); 
	/////////////////////////////////////
	strcat(data, "\"i\": \"0\","); 
	snprintf(tmp, BUF_SIZE, "%s%x%s", "\"icrc\": \"", crc, "\",");
	strcat(data, tmp); 
	snprintf(tmp, BUF_SIZE, "%s%d%s", "\"ilen\": \"", n, "\",");
	strcat(data, tmp); 
	snprintf(tmp, BUF_SIZE, "%s%d%s", "\"alllen\": \"", n, "\",");
	strcat(data, tmp); 
	snprintf(tmp, FILE_BUF_SIZE, "%s%s%s", "\"binddata\": \"", fb64, "\",");
	strcat(data, tmp); 
	////////////////////////////////////
	//strcat(data, "}],"); 
	strcat(data, "}"); 
	strcat(data, "}"); 

	snprintf(tmp, FILE_BUF_SIZE, "%s%s%d%s%s", buf, "Content-Length: ", strlen(data), "\r\n\r\n", data);

	len = write(connfd, tmp, strlen(tmp));
	if (len < 0) {
		printf("发送失败\n");
	} else {
		printf("发送成功，大小 %d\n", len);
	}

	len = recv(connfd, tmp, BUF_SIZE, MSG_WAITALL);
	if (len > 0)
		write(1, tmp, len);
	printf("\n");

	return 0;
}

int remote_test()
{
    struct hostent *hptr = NULL;
    char   *ptr, **pptr;
    char   str[256];

    if((hptr = gethostbyname(REMOTE_SERVER)) == NULL) {
        printf(" gethostbyname error for host:%s\n", ptr);
        return 0;
    }
	switch(hptr->h_addrtype) {
	case AF_INET:
		{
			struct sockaddr_in serverAdd;

			bzero(&serverAdd, sizeof(serverAdd));
			serverAdd.sin_family = AF_INET;
			serverAdd.sin_addr.s_addr = *(unsigned long *)hptr->h_addr;
			serverAdd.sin_port = htons(REMOTE_PORT);

			int connfd = socket(AF_INET, SOCK_STREAM, 0);
			int connResult = connect(connfd, (struct sockaddr *)&serverAdd, sizeof(serverAdd));

			if (connResult < 0) {
				printf("连接失败，errno ＝ %d\n",errno);
				close(connfd);
				return 1;
			}
			else
			{
				printf("连接成功\n");
				file_test(connfd, "test.zip");
				close(connfd);
			}
		}
		break;
	case AF_INET6:
		printf("Not support ipv6\n", str);
		break;
	default:
		printf("Unknown address type\n");
		break;
	}

    return 0;
}

int main(int argc, char **argv)
{
    //connect_test();
    remote_test();
    return 0;
}
