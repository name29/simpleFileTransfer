#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_LEN 10240

int main (int argc, char**argv){

	struct sockaddr_in sock_serv;
    	int sfd;
	int fd;
	char buf[BUFFER_LEN];
	off_t count=0;
	off_t m;
	long int n;
    	int l=sizeof(struct sockaddr_in);
	struct stat buffer;

	if (argc != 5)
	{
		printf("Error usage : %s <ip_serv> <port_serv> <filename> <server_filename>\n",argv[0]);
		exit(1);
	}

	char* serverfilename = argv[4];
	int port = atoi(argv[2]);
	char* ipaddr = argv[1];

	sfd = socket(PF_INET,SOCK_STREAM,0);
	if (sfd == -1)
	{
        	perror("socket fail");
		exit(2);
	}

	l=sizeof(struct sockaddr_in);
	memset(&sock_serv,0,l);

	sock_serv.sin_family=AF_INET;
	sock_serv.sin_port=htons(port);

	if (inet_pton(AF_INET,ipaddr,&sock_serv.sin_addr)==0)
	{
		printf("Invalid IP adress\n");
		exit(3);
	}

	if ((fd = open(argv[3],O_RDONLY))==-1)
	{
		perror("open fail");
		close(sfd);
		exit(4);
	}

	if (stat(argv[3],&buffer)==-1)
	{
		perror("stat fail");
		close(sfd);
		exit(5);
	}

	if(connect(sfd,(struct sockaddr*)&sock_serv,l)==-1)
	{
        	perror("conexion error\n");
		close(sfd);
		exit(5);
	}

	m=send(sfd,serverfilename,strlen(serverfilename)+1,MSG_NOSIGNAL);
	if(m==-1)
	{
		perror("send error");
		close(fd);
		close(sfd);
		exit(6);
	}

	while(1)
	{
		n=read(fd,buf,BUFFER_LEN);

		if (n==0 ) break;

		if(n==-1)
		{
			perror("read fails");
			close(fd);
			close(sfd);
			exit(7);
		}

		m=send(sfd,buf,n,MSG_NOSIGNAL);
		if(m==-1)
		{
			perror("send error");
			close(fd);
			close(sfd);
			exit(8);
		}
		count+=m;
	}

	close(sfd);
	return 0;
}
