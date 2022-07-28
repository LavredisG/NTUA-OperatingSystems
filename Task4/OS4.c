#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h> //hostent
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/select.h>

#define YELLOW "\033[33m"
#define RED "\033[31;1m"
#define CYAN "\033[36m"
#define GREEN "\033[32m"
#define WHITE "\033[37m"
#define MAX(a, b) ((a) > (b) ? (a) : (b))

//Function to count words of input
int count_words(char* input)
{
	int N = 1;
	for (int i = 0; input[i] != '\n'; i++)
	{
		if (input[i] == ' ') N++;
	}
	return N;
}

//Function to check for system calls
void syscall_error(int syscall, const char* text)
{
	if (syscall < 0)
	{
		perror(text);
		exit(1);
	}
}
const char help_msg[273] = "\033[36mThe list of available commands is:\nexit (Exits the program and terminates the connection with the server)\nhelp (Prints the list of available commands)\nget (Get data from server)\nN name surname reason (Request permission to leave your house during quarantine)\033[37m\n";
//Fuction to retrieve IP address from HOST name
char * HOST_to_IP(const char* HOST)
{
	struct hostent *host;
	struct in_addr **addr_list;

	if ((host = gethostbyname(HOST)) == NULL)
	{
		herror("Failed to get the host info!\n");
		exit(EXIT_FAILURE);
	}

	addr_list = (struct in_addr **) host->h_addr_list; //h_addr_list contains a list of addresses in network byte order, so we cast it in struct in_addr**and store it in addr_list

	return inet_ntoa(*addr_list[0]);
}

int main(int argc, char** argv)
{
	//Check Arguments
	int flag[2] = { 0, 0 }, host_i, port_i, debug = 0;
	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "--host"))
		{
			flag[0] = 1;
			host_i = i + 1; //Set index of Hostname in argv
		}
		if (!strcmp(argv[i], "--port"))
		{
			flag[1] = 1;
			port_i = i + 1; //Set index of Port number in argv
		}
		if (!strcmp(argv[i], "--debug")) debug = 1;
	}
	if (debug)
	{
		printf(YELLOW "[DEBUG] Debug mode enabled\n");
		if (flag[0]) printf(YELLOW "[DEBUG] HOSTNAME: %s\n", argv[host_i]);
		else printf(YELLOW "[DEBUG] HOSTNAME: lab4-server.dslab.os.grnetcloud.net\n");
		if (flag[1]) printf(YELLOW "[DEBUG] PORT: %s\n", argv[port_i]);
		else printf(YELLOW "[DEBUG] PORT: 18080\n");
	}

	//Creating Client
	int socket_fd;
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	syscall_error(socket_fd, RED "Failed to create socket!\n");

	const char* IP;
	//Get IP address
	if (flag[0]) IP = HOST_to_IP(argv[host_i]);
	else IP = HOST_to_IP("lab4-server.dslab.os.grnetcloud.net"); //Default

	struct sockaddr_in Server;
	Server.sin_family = AF_INET;
	if (flag[1]) Server.sin_port = htons(atoi(argv[port_i]));
	else Server.sin_port = htons(18080); //Default
	Server.sin_addr.s_addr = inet_addr(IP);

	syscall_error(connect(socket_fd, (struct sockaddr *) &Server, sizeof(Server)), RED "Failed to connect to Server!\n"); //Connect to Server
	if (flag[0]) printf(GREEN "Connection with server %s established.\n", argv[host_i]);
	else printf(GREEN "Connection with server lab4-server.dslab.os.grnetcloud.net established.\n");
	printf(WHITE "What is your request?\n");
	printf(WHITE "Run 'help' for more information\n");
	char input[50];

	while (1)
	{
		fd_set fdset; //fd_set is a fixed-size buffer
		int maxfd;

		FD_ZERO(&fdset); //Removes all desc from vector/set
		FD_SET(0, &fdset); //Put the file desc in the vector/set
		FD_SET(socket_fd, &fdset);
		struct timeval tv;
		maxfd = MAX(socket_fd, 0) + 1;
		tv.tv_sec = 5;
		tv.tv_usec = 0;


		int nin = read(0, input, sizeof(input));
		syscall_error(nin, "Failed to read input from user!\n");
		input[nin] = '\0';

		if ((!strncmp(input, "exit", 4)) && (strlen(input) == 5))
		{
			syscall_error(close(socket_fd), RED "Failed to close the socket!\n");
			if (debug) printf(YELLOW "[DEBUG] socket closed\n");
			if (flag[0]) printf(RED "Connection with server %s terminated.\n", argv[host_i]);
			else  printf(RED "Connection with server lab4-server.dslab.os.grnetcloud.net terminated.\n");
			exit(0);
		}
		else if (!strncmp(input, "help", 4) && (strlen(input) == 5))
		{
			syscall_error(write(1, help_msg, sizeof(help_msg)), "Failed to print help message!\n");
		}
		else if (!strncmp(input, "get", 3) && (strlen(input) == 4))
		{
			int n_read = 0;
			syscall_error(write(socket_fd, "get", 3), "Failed to write to server!\n");
			if (debug) printf(YELLOW "[DEBUG] sent get\n");
			char msg_from_server[30], buff[30];
			int rd_fd = select(maxfd, &fdset, NULL, NULL, &tv);
			syscall_error(rd_fd, "Failed to select!\n");
			if (rd_fd == 0)
			{
				if (debug) printf(YELLOW "[DEBUG] server timed out (get data)\n");
				printf(RED "Server timed out (get data)!\n");
				syscall_error(close(socket_fd), RED "Failed to close the socket!\n");
				if (flag[0]) printf(RED "Connection with server %s terminated.\n", argv[host_i]);
				else  printf(RED "Connection with server lab4-server.dslab.os.grnetcloud.net terminated.\n");
				exit(0);
			}
			n_read = read(socket_fd, msg_from_server, sizeof(msg_from_server));
			syscall_error(n_read, "Failed to read message from Server!\n");
			msg_from_server[n_read] = '\0';
			if ((n_read > 0) && (msg_from_server[n_read - 1] == '\n')) msg_from_server[n_read - 1] = '\0';
			if (debug) printf(YELLOW "[DEBUG] read %s\n", msg_from_server);
			if (debug) printf("-----------------------------------\n");
			time_t timestamp;
			int type, light;
			float temperature;
			sscanf(msg_from_server, "%d %d %f %ld", &type, &light, &temperature, &timestamp);
			printf(WHITE "Latest event:\n");

			switch (type)
			{
			case 0:
				printf("boot (0)\n");
				break;
			case 1:
				printf("setup (1)\n");
				break;
			case 2:
				printf("interval (2)\n");
				break;
			case 3:
				printf("button (3)\n");
				break;
			case 4:
				printf("motion (4)\n");
				break;
			}
			temperature /= 100.00;
			printf("Temperature is: %.2f\n", temperature);
			printf("Light level is: %d\n", light);
			struct tm ts;
			ts = *localtime(&timestamp);
			strftime(buff, sizeof(buff), "%Y-%m-%d %X", &ts);
			printf("Timestamp is: %s\n", buff);
		}
		int N = count_words(input), n_read = 0; //n_ack = 4 + nin;//n_ack is for the end of ack array (because for some reason it kept reading useless bytes)
		char ver[100], msg[100], ack[100];
		if (N == 4) //N name username reason
		{
			int j = 0;
			while (input[j] != '\n')
			{
				j++;
			}
			input[j] = '\0';
			syscall_error(write(socket_fd, input, sizeof(input)), "Failed to write request to server!\n");

			if (debug) printf(YELLOW "[DEBUG] sent %s\n", input);//memory doesn't flush

			int rd_fd = select(maxfd, &fdset, NULL, NULL, &tv);
			syscall_error(rd_fd, "Failed to select!\n");
			if (rd_fd == 0)
			{
				if (debug) printf(YELLOW "[DEBUG] server timed out (Verification code)\n");
				printf(RED "Server timed out (Verification code)!\n");
				syscall_error(close(socket_fd), RED "Failed to close the socket!\n");
				if (flag[0]) printf(RED "Connection with server %s terminated.\n", argv[host_i]);
				else  printf(RED "Connection with server lab4-server.dslab.os.grnetcloud.net terminated.\n");
				exit(0);
			}

			n_read = read(socket_fd, msg, sizeof(msg)); //Read either msg = "try again" if N not a number or msg = verification code
			syscall_error(n_read, "Failed to read message from server!\n");
			msg[n_read] = '\0';
			if (msg[n_read - 1] == '\n') msg[n_read - 1] = '\0';

			if (!strncmp(msg, "try again", 9))
			{
				if (debug) printf(YELLOW "[DEBUG] read %s\n", msg);
				printf(WHITE "try again\n");
				continue;
			}

			if (debug) printf(YELLOW "[DEBUG] read %s\n", msg);

			printf(WHITE "Send verification code: %s\n", msg);
			n_read = read(0, ver, sizeof(ver)); 				//Read ver code from user
			syscall_error(n_read, "Failed to read verification code!\n");
			ver[n_read] = '\0';
			if (ver[n_read - 1] == '\n') ver[n_read - 1] = '\0';
			syscall_error(write(socket_fd, ver, sizeof(ver)), "Failed to sent verification code to server!\n"); //Send ver code to server
			if (debug) printf(YELLOW "[DEBUG] sent %s\n", ver);

			int rd_fd2 = select(maxfd, &fdset, NULL, NULL, &tv);
			syscall_error(rd_fd2, "Failed to select!\n");
			if (rd_fd2 == 0)
			{
				if (debug) printf(YELLOW "[DEBUG] server timed out (Acknowledgement)\n");
				printf(RED "Server timed out (Acknowledgement)!\n");
				syscall_error(close(socket_fd), RED "Failed to close the socket!\n");
				if (flag[0]) printf(RED "Connection with server %s terminated.\n", argv[host_i]);
				else  printf(RED "Connection with server lab4-server.dslab.os.grnetcloud.net terminated.\n");
				exit(0);
			}
			int n_ack = read(socket_fd, ack, sizeof(ack));
			syscall_error(n_ack, "Failed to read acknowledgement from server!\n"); //Read ACK message or 'invalid code' from server
			int i = 0;
			while (ack[i] != '\n')
			{
				i++;
			}
			ack[i] = '\0';
			if (!strncmp(ack, "invalid code", 12))
			{
				if (debug) printf(YELLOW "[DEBUG] read %s\n", ack);
				printf(WHITE "invalid code\n");
				continue;
			}
			if (debug) printf(YELLOW "[DEBUG] read %s\n", ack);
			printf(WHITE "Response: %s\n", ack);
		}
		memset(input, 0, sizeof(input));
	}
	return 0;
}
