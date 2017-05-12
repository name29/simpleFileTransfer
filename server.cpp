#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <fcntl.h>
#include <unistd.h>

#define RECV_BUFFER_LEN 10240
#define MAX_FILENAME 512
#define BACKLOG 100


// Security todo:
// Add encryption during trasnfer
// Add authentication
// Verify that filename is valid:
//	within chroot
//	without shell injection
// Run inside chroot
// Drop privileges
// Add log

int main(int argc,char** argv)
{
	int socket_fd;
	int client_socket_fd;
	int uploadedfile_fd;

	int tcp_port;
	int yes=1;

	struct sockaddr_in sockaddr_in_server;
	struct sockaddr_in sockaddr_in_client;

	unsigned int length=sizeof(struct sockaddr_in);
	ssize_t received_bytes;
	int l;

	int client_tcp_port;

	char recv_buffer[RECV_BUFFER_LEN];
	char* rrecv_buffer;
	char command_buffer[256];

	char* cfilename;
	char* dirpath;

	char* script_name;

	char client_address[INET_ADDRSTRLEN];
	char filename[MAX_FILENAME];
	int filename_index;

	if(argc!=3)
	{
        	printf("Usage: %s <num_tcp_port> <script_name>\n",argv[0]);
	        exit(1);
	}
	tcp_port = atoi(argv[1]);
	script_name = argv[2];


	/*SETUP SOCKET */
	socket_fd = socket(PF_INET,SOCK_STREAM,0);
	if (socket_fd == -1)
	{
	        perror("Unable to create socket");
        	exit(2);
	}

	if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,&yes,sizeof(int)) == -1 )
	{
	        perror("setsockopt errors");
		close(socket_fd);
	        exit(3);
	}

	l=sizeof(struct sockaddr_in);
	memset(&sockaddr_in_server,0,l);

	sockaddr_in_server.sin_family=AF_INET;
	sockaddr_in_server.sin_port=htons(tcp_port);
	sockaddr_in_server.sin_addr.s_addr=htonl(INADDR_ANY);

	if(bind(socket_fd,(struct sockaddr*)&sockaddr_in_server,l)==-1)
	{
		perror("bind fail");
		close(socket_fd);
		exit(4);
	}

	memset(recv_buffer,0,RECV_BUFFER_LEN);

	if ( listen(socket_fd,BACKLOG) != 0 )
	{
		perror("listen fail");
		close(socket_fd);
		exit(5);
	}

	/* CONNECTION LOOP */
	while ( 1 )
	{
		/*WAITING FOR CONNECTION */

		printf("Waiting for connection\n");
		client_socket_fd=accept(socket_fd,(struct sockaddr*)&sockaddr_in_client,&length);
		if(client_socket_fd==-1)
		{
		        perror("accept error");
			close(socket_fd);
			exit(6);
		}
	        if(inet_ntop(AF_INET,&sockaddr_in_client.sin_addr,client_address,INET_ADDRSTRLEN)==NULL)
		{
	        	perror("inet_ntop error");
			close(client_socket_fd);
			continue;
	        }

	        client_tcp_port=ntohs(sockaddr_in_client.sin_port);
        	printf("Received connection from : %s:%d\n",client_address,client_tcp_port);

		uploadedfile_fd = -1;
		filename_index = 0;
		/*RECEIVING LOOP */
        	while(1)
		{
	        	received_bytes=recv(client_socket_fd,recv_buffer,RECV_BUFFER_LEN,0);
			rrecv_buffer = recv_buffer;
	        	if ( received_bytes == 0 ) break;

			if(received_bytes==-1)
			{
                		perror("recv failed");
				close(client_socket_fd);
				break;
        		}

			/* IF THE OUTPUT FILE IS NOT ALREADY OPEN, LOOK FOR FILENAME (ended by '\0') */
			if ( uploadedfile_fd < 0 )
			{
				//Reading file name
				for ( int i = 0 ; i < received_bytes ; i++ )
				{
					/*Copy bytes inside filename char array */
					if ( filename_index < MAX_FILENAME-1  && recv_buffer[i] != '\0' )
					{
						filename[filename_index] = recv_buffer[i];
					}
					else
					{
						/*If maximum length reached or '\0' found */

						filename[filename_index] = '\0'; //Add '\0'

						cfilename = strdup(filename); //Copy in order to parse the directory
						dirpath = dirname(cfilename); // dirname modify cfilename

						printf("Creating the output dirpath : %s\n",dirpath);

						sprintf(command_buffer,"mkdir -p %s\n",dirpath); //Create the command for mkdir -p
						system(command_buffer); //Execute command
						free(cfilename); //Release strdump memory

						printf("Creating the copied output file : %s\n",filename);

						//Open file
						if ((uploadedfile_fd=open(filename,O_CREAT|O_WRONLY,0774))==-1)
						{
        						perror("open fail");
							close(client_socket_fd);
							client_socket_fd=-1; /* open failed flag */
							break;
						}
						//Shift right the buffer readed by write
						rrecv_buffer = recv_buffer+i+1;
						received_bytes -= i+1;
						break;

					}
					filename_index++;
				}
				/* open failed */
				if ( client_socket_fd < 0 ) break;
			}

			//If there are any bytes to write and the output file is already created
			if ( uploadedfile_fd >= 0 && received_bytes > 0 )
			{
				//Write
	        		if(write(uploadedfile_fd,rrecv_buffer,received_bytes) != received_bytes)
				{
			                perror("write fail");
					close(client_socket_fd);
					close(uploadedfile_fd);
					break;
			        }
			}
		}

		if (received_bytes != 0 )
		{
			printf("Error during last connection\n");
		}

		if (uploadedfile_fd >= 0 )
		{
	        	close(uploadedfile_fd);
		}
		close(client_socket_fd);

		//Execute the external script
		sprintf(command_buffer,"./%s %s",script_name,filename);
		system(command_buffer);
        }
	close(socket_fd);

	return 0;
}
