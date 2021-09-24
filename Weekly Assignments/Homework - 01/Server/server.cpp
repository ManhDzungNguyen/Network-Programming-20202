#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "winsock2.h"
#include "ws2tcpip.h"
#include "stdio.h"
#include "conio.h"
#include <string>

#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048
#pragma comment(lib, "Ws2_32.lib")


/**
 * Function: IPtoDNS
 * Desscription: convert an IPv4 address to a domain name
 * Param	IPaddr		A pointer to a buffer containing the IPv4 address
 * Return	A pointer to a buffer containing list of domain names, separated by " "
 */
char* IPtoDNS(char* IPaddr) {
	unsigned long ip = inet_addr(IPaddr);
	HOSTENT *host = gethostbyaddr((const char FAR*)&ip, sizeof in_addr, 0);
	char *buff = new char[BUFF_SIZE];
	strcpy_s(buff, BUFF_SIZE, "");
	if (host == NULL) {
		strcat_s(buff, BUFF_SIZE, "-"); // Add prefix "-" in message to help client realize the error message
		strcat_s(buff, BUFF_SIZE, "gethostbyaddr failed with error: ");
		char errorMessage[BUFF_SIZE];
		sprintf_s(errorMessage, BUFF_SIZE, "%d", WSAGetLastError());
		strcat_s(buff, BUFF_SIZE, errorMessage);
	}
	else {
		strcat_s(buff, BUFF_SIZE, "+"); // Add prefix "+" in message to help client realize the message contain result 
		strcat_s(buff, BUFF_SIZE, host->h_name);

		if (host->h_aliases[0] != NULL) 
			for (int count = 0; host->h_aliases[count] != 0; ++count) {
				strcat_s(buff, BUFF_SIZE, " ");
				strcat_s(buff, BUFF_SIZE, host->h_aliases[count]);
			}
		else {
			strcat_s(buff, BUFF_SIZE, " ");
			strcat_s(buff, BUFF_SIZE, "NULL");
		}
	}
	return buff;
}


/**
* Function: DNStoIP
* Desscription: convert a domain name to an IPv4 address
* Param		DNS		A pointer to a buffer containing the domain name
* Return	A pointer to a buffer containing list of IPv4 addrress, separated by " "
*/
char* DNStoIP(char* DNS) {
	ADDRINFO* result;
	int rc;

	ADDRINFO hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;

	rc = getaddrinfo(DNS, NULL, &hints, &result);
	char *buff = new char[BUFF_SIZE];
	strcpy_s(buff, BUFF_SIZE, "");

	if (rc == 0) {
		strcat_s(buff, BUFF_SIZE, "+"); // Add prefix "+" in message to help client realize the message contain result
		SOCKADDR_IN* address;
		char ipStr[INET_ADDRSTRLEN];

		int countIP = 0;
		for (addrinfo* res = result; res != NULL; res = res->ai_next) {
			countIP++;
			address = (struct sockaddr_in*)res->ai_addr;
			inet_ntop(AF_INET, &address->sin_addr, ipStr, sizeof(ipStr));
			strcat_s(buff, BUFF_SIZE, ipStr);
			strcat_s(buff, BUFF_SIZE, " ");
		}
		if (countIP == 1) {
			strcat_s(buff, BUFF_SIZE, "NULL");
		}
	}
	else {
		strcat_s(buff, BUFF_SIZE, "-"); // Add prefix "-" in message to help client realize the error message
		strcat_s(buff, BUFF_SIZE, "getaddrinfo failed with error: ");
		char errorMessage[BUFF_SIZE];
		sprintf_s(errorMessage, BUFF_SIZE, "%d", WSAGetLastError());
		strcat_s(buff, BUFF_SIZE, errorMessage);
	}

	freeaddrinfo(result);
	return buff;
}

int main(int argc, char *argv[])
{
	
	// Step 1: Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("WinSock 2.2 is not supported\n");
		return 0;
	}

	// Step 2: Construct socket
	SOCKET server;
	server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (server == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket.", WSAGetLastError());
		return 0;
	}

	// Step 3: Bind address to socket
	const int SERVER_PORT = atoi(argv[1]);
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);
	if (bind(server, (SOCKADDR*)&serverAddr, sizeof(serverAddr))) {
		printf("Error %d: Cannot bind this address.", WSAGetLastError());
		_getch();
		return 0;
	}
	printf("Server started!");

	// Step 4: Communicate with client
	SOCKADDR_IN clientAddr;
	char buff[BUFF_SIZE], clientIP[INET_ADDRSTRLEN];
	int ret, clientAddrLen = sizeof(clientAddr), clientPort;

	while (1) {
		// Receive request
		ret = recvfrom(server, buff, BUFF_SIZE, 0, (SOCKADDR*)&clientAddr, &clientAddrLen);
		if (ret == SOCKET_ERROR)
			printf("Error %d: Cannot receive data", WSAGetLastError());
		else if (strlen(buff) > 0) {
			buff[ret] = 0;
			inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
			clientPort = ntohs(clientAddr.sin_port);
			printf("Receive from client [%s:%d]: %s\n", clientIP, clientPort, buff);

			// Processing request
			char* result;
			struct sockaddr_in sa;
			int checkIP = inet_pton(AF_INET, buff, &(sa.sin_addr)); 
			// check input string is an IP address or not
			if (checkIP == 1)
				result = IPtoDNS(buff);
			else
				result = DNStoIP(buff);

			// Echo to client
			ret = sendto(server, result, strlen(result), 0, (SOCKADDR*)&clientAddr, sizeof(clientAddr));
			if (ret == SOCKET_ERROR)
				printf("Error %d: Cannot send data", WSAGetLastError());
		}
	} // end while

	// Step 5: Close socket
	closesocket(server);

	// Step 6: Terminal Winsock
	WSACleanup();

	return 0;
}

