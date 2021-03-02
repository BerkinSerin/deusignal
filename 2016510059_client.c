#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <semaphore.h>

int clientSocket;
char username[100];
char groupname[100];
void *receive(void *sockID)
{
	int clientSocket = *((int *)sockID);
	while (1)
	{
		char rec[20000];
		// receive messages from the server
		int final = recv(clientSocket, rec, 20000, 0);
		rec[final] = '\0';
		printf("%s\n", rec);

		// keep the username as a variable for the clients
		if (strstr(rec, "-username") != NULL)
		{

			char temp[100];
			char temp2[100];
			strcpy(temp, rec);
			strcpy(temp2, strtok(temp, " "));
			strcpy(temp2, strtok(NULL, " "));
			strcpy(username, temp2);
		}
		// keep the groupname as a variable for the clients
		else if (strstr(rec, "-groupname") != NULL)
		{

			char temp[100];
			char temp2[100];
			strcpy(temp, rec);
			strcpy(temp2, strtok(temp, "+"));
			strcpy(temp2, strtok(NULL, "+"));
			strcpy(groupname, temp2);
		}
		// if server send exit message terminate the program
		else if (strstr(rec, "-exit") != NULL)
		{
			exit(0);
		}
	}
}
int main()
{

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in serverAddr;

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(3205);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
		return 0;

	printf("Connected to the DEU Signal!!!!\n");

	pthread_t thread;
	pthread_create(&thread, NULL, receive, (void *)&clientSocket);

	while (1)
	{
		char input[100];
		gets(input);
		// if the input is -send command send that command as a json message
		if (strstr(input, "-send") != NULL)
		{
			char json[100];
			sprintf(json, "{\"from\":\"%s\",\"to\":\"%s\", \"message\":\"%s\"}", username, groupname, strstr(input, " "));
			sprintf(input, "-send %s", json);
			send(clientSocket, input, 100, 0);
		}
		else
		{
			send(clientSocket, input, 100, 0);
		}
	}
	return 0;
}