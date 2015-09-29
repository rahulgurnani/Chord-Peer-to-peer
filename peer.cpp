/* Authors : 
	Rahul Gurnani, Ashutosh Baheti */

/* contains the main function and the code that executes on each node*/
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include "node.hxx"
#include <map>
#include <fcntl.h> 

#define PORT 10001
#define PORT_OFFSET 1000
#define BUFF_LEN 1000 		//Max length of buffer

int sock_fd;
Node self;
char buff[BUFF_LEN];
char buff1[BUFF_LEN];
map<string, int> file_to_port;   	 			// given a file_id gives the port number
map<string, string> file_to_path;   	 		// given a file_id gives the port number

void error(string msg)
{
	perror(msg.c_str());
	exit(EXIT_FAILURE);
}

void ctrl_c_handler(int dummy=0)
{
	close(sock_fd);
	exit(0);
}

void send_to_node(int port, string s)
{

	struct sockaddr_in si_other;			// si_other for the FIS 
	int sock_fd_FIS, slen=sizeof(si_other);
	// create new datagram socket with UDP connection
	if ( (sock_fd_FIS=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}
	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(port);
	// inet_aton converts the localhost into binary form and stores it in sin_addr
	if (inet_aton("127.0.0.1" , &si_other.sin_addr) == 0)
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	bzero(buff,BUFF_LEN);
	std::strcpy (buff, s.c_str());
	if (sendto(sock_fd_FIS, buff, strlen(buff) , 0 , (struct sockaddr *) &si_other, slen)==-1)
	{
		perror("sendto()");
		exit(EXIT_FAILURE);
	}
	printf("sent %s to %d\n",s.c_str(),port);
}
bool in_between(unsigned long long m1, unsigned long long m2, unsigned long long m)
{
	if(m==m1)
	{
		perror("Same port Number ");
		return false;
	}
	if((m>m1 && m<m2) || m==m2)
	{
		return true;
	}
	if(m1>=m2 && (m > m1 || m <= m2))
	{
		return true;
	}
	return false;
}

// current is the current searcing node, n is the node to be inserted
void insert_node(char* n, Node& current)
{
	int port = atoi(n);
	string port_s(n);
	printf("inserting node = %s\n",n );
	printf("Current Self Port : %d\n",current.get_port());
	bzero(buff1,BUFF_LEN);
	sprintf(buff1,"127.0.0.1:%d",port);
	string temp(buff1);
	unsigned long long machine_id = oat_hash(temp);
	printf("Inserting MID = %llu\n",machine_id);
	if(in_between(current.get_mid(), current.get_successor().get_mid(),machine_id) || ( current.get_successor().get_mid() == current.get_mid()))
	{
		// setting successor for the newly added node
		
		
		sprintf(buff,"P%d",current.get_port());
		send_to_node(port,string(buff));
		
		sprintf(buff,"S%d",current.get_successor().get_port());
		
		send_to_node(port,string(buff));
		

		// updating predecessor of current's successor and updating successor of current
	//	printf("%s\n",port_s.c_str());
		send_to_node(current.get_successor().get_port(),"P"+port_s);
		info dummy("127.0.0.1",port_s);
		current.set_successor(dummy);
		printf("Current successor : %d\n",current.get_successor().get_port());
		printf("Current predecessor : %d\n",current.get_predecessor().get_port());
		printf("Current Port :%d\n",current.get_port() );
		printf("Machine ID : %llu\n",current.get_mid());
	}
	else
	{
		// check in the successor
		send_to_node(current.get_successor().get_port(), "A"+string(n));
	}
}

/******** SEND_FILE() *********************
There is a separate instance of this function 
for each connection.  It handles all communication
once a connnection has been established.
*****************************************/
void send_file(int sock)
{
	int n;
	char file_path[256];

	bzero(buff, BUFF_LEN);
	bzero(file_path,256);

	n = read(sock,file_path,255);
	// get file path from file_path
	if (n < 0) 
		error("ERROR reading from socket");

	// loading the file
	printf("Sending file with file path : %s  \n",file_path);

	int fd_read = open(file_path,O_RDONLY);
	if(fd_read!=-1) // read succesful
	{ 
		printf("file found!! \n");
		int s = 0;
		do{
			bzero(buff,BUFF_LEN);
			s = read(fd_read,buff, BUFF_LEN );
			int k = write(sock, buff,s);
			if(k<0)
				error("ERROR writing to socket");
			
		}while(s!=0);
	}
	printf("Reached the end\n");
	close(fd_read);
	close(sock);
}

void download_file(string file_path,int server_port)
{
	// declarations
	string file_name(file_path.substr(file_path.find_last_of('/')+1));
	char buff[BUFF_LEN];
	int n,sock_fd_peer;
	struct sockaddr_in serv_addr;
	// setting up connection with the peer
	sock_fd_peer = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd_peer < 0) 
		error("ERROR opening socket");
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(server_port);
	inet_aton("127.0.0.1",&serv_addr.sin_addr);

	
	if (connect(sock_fd_peer,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
		error("ERROR connecting");
	
	n = write(sock_fd_peer,file_path.c_str(),file_path.length());
	if (n < 0) 
		error("ERROR writing to socket");

	// create file
	int fd_write = open(file_name.c_str(), O_WRONLY | O_CREAT| O_TRUNC); // creating a new file with given file name

	// download file
	int s = 0;
	do{
		bzero(buff,BUFF_LEN);
		s = read(sock_fd_peer, buff, BUFF_LEN); 		// reading from socket
		write(fd_write, buff, s); 				// write to file with file descriptor fd_write
		

	} while(s!=0);
	close(sock_fd_peer);
	close(fd_write);
	printf("file closed!\n");
}

int main(int argc, char const *argv[])
{
	struct in_addr ip_addr;
	int pid;
	signal(SIGINT, ctrl_c_handler);
	inet_aton("127.0.0.1",&ip_addr);
	printf("The IP address is %s\n", inet_ntoa(ip_addr));
	if(argc < 2)
	{
		// Invalid case
		perror("Enter the Port Number!!");
		exit(EXIT_FAILURE);
	}
	// successor = predecessor = self;
	self = Node(ip_addr.s_addr,atoi(argv[1]));
	
	if(argc == 2)
	{
		// currently the only one in the peer network
		info dummy(self.get_ip(),self.get_port());
		self.set_successor(dummy);
		self.set_predecessor(dummy);
		printf("Only peer : %llu\n",self.get_mid());
	}
	
	

	//create fork and start a server in the child
	if((pid = fork()) == -1)
	{
		perror("Fork error!");
		exit(EXIT_FAILURE);
	}
	else if(pid == 0)
	{
		// initiate a server for downloading
		int server_pid;
		if((server_pid = fork()) == -1)
		{
			perror("Download file server");
			exit(EXIT_FAILURE);
		}
		else if(server_pid == 0)
		{
			// server code
			int newsockfd, clilen;
			//Server address and client address
			struct sockaddr_in serv_addr, cli_addr;

			signal(SIGINT, ctrl_c_handler);

			//Create Server Socket
			int sockfd = socket(AF_INET, SOCK_STREAM, 0);
			if (sockfd < 0)
				error("ERROR opening socket");
			
			bzero((char *) &serv_addr, sizeof(serv_addr));

			serv_addr.sin_family = AF_INET;
			serv_addr.sin_addr.s_addr = INADDR_ANY;
			serv_addr.sin_port = htons(self.get_port() + PORT_OFFSET);

			if (bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
				error("ERROR on binding");
			listen(sockfd,5);
			
			printf("File transfer server is up!!\n");
			clilen = sizeof(cli_addr);
			while (1)
			{
				newsockfd = accept(sockfd,(struct sockaddr *)&cli_addr,(socklen_t *)&clilen);
				printf("Connection Accepted!!\n");
				if (newsockfd < 0)
					error("ERROR on accept");
				pid = fork();
				if (pid < 0)
					error("ERROR on fork");
				else if (pid == 0)
				{
					close(sockfd);
					// send file here
					send_file(newsockfd);
					exit(0);
				}
				else close(newsockfd);
			} /* end of while */
			exit(EXIT_SUCCESS);
		}

		// reciever is here
		struct sockaddr_in si_me, si_other;
		int slen = sizeof(si_other) , recv_len;
		//create a UDP socket
		if ((sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		{
			perror("Unable to create socket");
			exit(EXIT_FAILURE);
		}

		// zero out the structure
		memset((char *) &si_me, 0, sizeof(si_me));

		// AF_INET signifies internet family
		si_me.sin_family = AF_INET;
		// Convert port number for Host TO Network Short
		si_me.sin_port = htons(self.get_port());
		// Covert the global address of allowing any user to send from Host TO Network Long
		si_me.sin_addr.s_addr = htonl(INADDR_ANY);

		// bind socket to port
		// we have to wait for the incoming connections. Therefore, there is a need to bind the socket to the port
		// sock_fd - socket file discriptor which we get when we create the socket
		// si_me - socket address stucture which we have just initialized
		// size of the structure
		if( bind(sock_fd , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
		{
			perror("Bind error!");
			exit(EXIT_FAILURE);
		}


		while(true)
		{
			printf("waiting to recieve anything \n");
			bzero(buff,BUFF_LEN);
			if ((recv_len = recvfrom(sock_fd, buff, BUFF_LEN, 0, (struct sockaddr *) &si_other,(socklen_t *) &slen)) == -1)
			{
				perror("recvfrom()");
				exit(EXIT_FAILURE);
			}
			printf("received = %s\n",buff );
			if(buff[0]=='A')
			{
				// add new node
				printf("Request received\n");
				
				insert_node(buff+1,self);
			}
			else if(buff[0]=='L')
			{
				// locate file
				// used for uploading a file
				// after last hash there is filename
				unsigned loc = string(buff).find_last_of("#");
				string file_path = string(buff).substr(1,loc-1); 		// might be erroeneous
				
				string file_name(file_path.substr(file_path.find_last_of('/')+1));
				
				unsigned long long file_id = oat_hash(file_name);
				
				
				if(in_between(self.get_predecessor().get_mid(),self.get_mid(),file_id))
				{
					int port = atoi((string(buff).substr(loc+1)).c_str());
					printf("Located file : port = %d , %s, file id = %llu, predecessor id = %llu, current id = %llu \n",port, file_name.c_str(), file_id, self.get_predecessor().get_mid(),self.get_mid());
					file_to_port[file_name] = port;
					file_to_path[file_name] = file_path;
				}
				else
				{
					printf("Not of this machine, going to next \n");
					send_to_node(self.get_successor().get_port(),buff);
				}
			}
			else if(buff[0]=='S')
			{
				// set successor
				info dummy("127.0.0.1", string(buff+1));
				self.set_successor(dummy);
				printf("new successor : %s\n",buff+1 );
			}
			else if(buff[0]=='P')
			{
				// set predecessor
				info dummy("127.0.0.1", string(buff+1));
				self.set_predecessor(dummy);
				printf("new predecessor : %s\n",buff+1 );
			}
			else if(buff[0]=='U')
			{
				// upload file
				string file_name = string(strrchr(buff,'/')+1);
				string file_path = string(buff+1);
				printf("Recieved File Name = %s \n",file_name.c_str());
				unsigned long long file_id = oat_hash(file_name);
				
				if(in_between(self.get_predecessor().get_mid(),self.get_mid(),file_id))
				{
					int port = self.get_port();
					printf("Located file : %s, file id = %llu, predecessor id = %llu, current id = %llu \n",file_name.c_str(), file_id, self.get_predecessor().get_mid(),self.get_mid());
					file_to_port[file_name] = port;
					file_to_path[file_name] = file_path;
				}
				else
				{
					bzero(buff1,sizeof(buff1));
					sprintf(buff1,"L%s#%d",file_path.c_str(),self.get_port());
					string msg(buff1);
					printf("Not of this machine, going to next\n ");
					send_to_node(self.get_successor().get_port(),msg);
				}
			}
			else if(buff[0]=='R')
			{
				// Relay request

				string file_path(strrchr(buff,'#')+1);
				int port,dest_port;
				sscanf(buff+1,"%d",&port);
				sscanf(strchr(buff,'#')+1,"%d",&dest_port);
				if(port == self.get_port())
				{
					if(dest_port == -1)
					{
						printf("%s File not found\n",file_path.c_str());
					}
					else
					{
						// download file code
						printf("File found at port %d current_port %d\n",dest_port,port);
						// fork and download file
						if((pid = fork()) == -1)
						{
							perror("File downloader fork error");
							exit(EXIT_FAILURE);
						}
						else if(pid == 0)
						{
							printf("Downloading File from name : %s : %d\n",file_path.c_str(),(dest_port+PORT_OFFSET));
							download_file(file_path,dest_port+PORT_OFFSET);
						}
					}
				}
				else
				{
					// send relay to successor
					bzero(buff1,BUFF_LEN);
					strcpy(buff1,buff);
					send_to_node(self.get_successor().get_port(),string(buff1));
				}
			}
			else if(buff[0] == 'F')
			{
				// convention for find file request is "F"+port+"#"+file_name
				// find file request
				string file_name(strrchr(buff,'#')+1);
				// get the requesting port
				int port,dest_port;
				sscanf(buff+1,"%d",&port);
				unsigned long long file_id = oat_hash(file_name);
				if(in_between(self.get_predecessor().get_mid(), self.get_mid(), file_id))
				{
					// if file_id is located on this machine then try finding the file
					if(file_to_port.find(file_name) == file_to_port.end())
					{
						// file not found
						bzero(buff1,sizeof(buff1));
						sprintf(buff1,"R%d#%d#%s",port,-1,file_name.c_str());
						string msg(buff1);
						send_to_node(self.get_port(),msg);
					}
					else
					{
						dest_port = file_to_port.find(file_name)->second;
						// relay this information to the sender
						bzero(buff1,sizeof(buff1));
						sprintf(buff1,"R%d#%d#%s",port,dest_port,file_to_path[file_name].c_str());
						string msg(buff1);
						send_to_node(self.get_port(),msg);
					}
				}
				else
				{
					//find file in the successor
					string msg(buff);
					send_to_node(self.get_successor().get_port(),msg);
				}
			}
			else if(buff[0]=='D')
			{
				// we have got the destination machine this machine contains where the file actually 
				string temp = string(buff+2);
				int loc = temp.find_first_of("#");
				string file_name = temp.substr(0,loc);
				int dest_port = file_to_port[file_name];
				printf("destination port found is %d \n",dest_port);
				// left here 
			}
		}
		exit(EXIT_SUCCESS);
	}

	usleep(100);
	if(argc == 3)
	{
		// it has to connect to the peer network
		printf("connecting to %s\n",argv[2]);
		bzero(buff1,BUFF_LEN);
		sprintf(buff1,"A%d",self.get_port());
		string msg(buff1);
		send_to_node(atoi(argv[2]),msg);
	}
	// Client code is here
	// Here the client will upload and search for files
	while(1)
	{
		int choice;
		signal(SIGINT, ctrl_c_handler);
		printf("Choose your option:\n1. Upload a file to the network\n2. Search for a file in the netwrok\n");
		scanf("%d",&choice);
		if(choice < 1 || choice > 2)
		{
			perror("Invalid Option!!");
		}
		else if(choice == 1)
		{
			// upload a file
			FILE *fp;
			fp = popen("python2 file.py", "r");
			if (fp == NULL)
			{
				printf("Failed to run command\n" );
				exit(1);
			}

			/* Read the output a line at a time - output it. */
			while (fgets(buff, sizeof(buff)-1, fp) != NULL)
			{
				buff[strlen(buff)-1] = '\0';
				printf("Selected File :%s", buff);
			}
			send_to_node(self.get_port(),"U"+string(buff));
			//send the file to the server to upload it
		}
		else if(choice == 2)
		{
			printf("Enter the filename to be searched :\n");
			bzero(buff,sizeof(buff));
			getchar();
			scanf("%[^\n]",buff);
			bzero(buff1,sizeof(buff1));
			sprintf(buff1, "F%d#%s",self.get_port(),buff);
			send_to_node(self.get_port(), string(buff1));
			// send the file to the server to search it

		}
	}

	return 0;
}

