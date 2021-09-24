#include "stdafx.h"
#include "stdio.h"
#include "conio.h"
#include "winsock2.h"
#include "ws2tcpip.h"

#include <iostream>
#include <cstring>
#include <vector>

#define BUFF_SIZE 2048
#pragma comment(lib, "Ws2_32.lib")

/**
* Function: DNStoIP
* Desscription: decrypt the Server response and show the result
* Param		buff	A pointer to a buffer containing the Server message
* Param		isIPv4	a boolean show the request to server is IPv4 or not
*/
void decryptServerMesssage(char* buff, bool isIPv4) {
	// Check the message from server is an error message or not
	if (buff[0] == '-')
		printf("Not found information\n");
	else if (buff[0] == '+') {
		buff++;

		// Tokenize the server message
		std::vector<char*> v;
		char *next_token = NULL;
		char* token = strtok_s(buff, " ", &next_token);

		while (token != NULL) {
			v.push_back(token);
			token = strtok_s(NULL, " ", &next_token);
		}

		for (size_t n=0; n<v.size(); n++){
			if (!isIPv4) {
				if (n == 0) {
					printf("Official IP: %s\n", v[n]);
					printf("Alias IP:\n");
				}
				else printf("%s\n", v[n]);
			}
			else {
				if (n == 0) {
					printf("Official name: %s\n", v[n]);
					printf("Alias name:\n");
				}
				else printf("%s\n", v[n]);
			}

		}
	}
	else
		printf("Can not read the message from server!\n");
	printf("\n");
}


int main(int argc, char* argv[])
{
	// Step 1: Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("WinSock 2.2 is not supported\n");
		return 0;
	}

	// Step 2: Construct socket
	SOCKET client;
	client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket.", WSAGetLastError());
		return 0;
	}

	printf("Client started!\n");

	// Set time-out for receiving
	int tv = 10000; //Time-out interval: 10000ms
	setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv), sizeof(int));

	// Step 3: Specify server address
	const char* SERVER_ADDR = argv[1];
	const int SERVER_PORT = atoi(argv[2]);
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	// Step 4: Communicate with server
	char buff[BUFF_SIZE];
	int ret, serverAddrLen = sizeof(serverAddr), messageLen;

	while (1) {
		// Send request to server
		printf("Send request to server: ");
		gets_s(buff, BUFF_SIZE);
		messageLen = strlen(buff);

		// check input string is an IP address or not
		struct sockaddr_in sa;
		bool isIPv4;
		int checkIP = inet_pton(AF_INET, buff, &(sa.sin_addr));
		if (checkIP == 1)
			isIPv4 = true;
		else
			isIPv4 = false;

		// Stop when client input is a NULL string
		if (messageLen == 0) break;

		ret = sendto(client, buff, messageLen, 0, (SOCKADDR *)&serverAddr, serverAddrLen);

		if (ret == SOCKET_ERROR)
			printf("Error %d: Cannot send mesage.", WSAGetLastError());

		// Receive echo message
		ret = recvfrom(client, buff, BUFF_SIZE, 0, (SOCKADDR *)&serverAddr, &serverAddrLen);

		if (ret == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT)
				printf("Time-out!");
			else printf("Error %d: Cannot receive mesage.", WSAGetLastError());
		}
		else if (strlen(buff) > 0) {
			buff[ret] = 0;
			decryptServerMesssage(buff, isIPv4); // decrypt Server Message and show Output
		}
	} // end while

	  // Step 5: Close socket
	closesocket(client);

	// Step 6: Terminate Winsock
	WSACleanup();

	return 0;
}