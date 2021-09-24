#include "stdafx.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <vector>
#include <iostream>
using namespace std;

#define SERVER_ADDR "127.0.0.1"
#define HEADER_SIZE 16
#define MAX_CLIENT 10

#pragma comment(lib, "Ws2_32.lib")


/**
* Function: send_finetune
* Description: an improved version of send function, which helps you to send
*				the message included the size of data.
* Param		s		A descriptor identifying a connected socket
* Param		data	the data to be transmitted
*/
int send_finetune(SOCKET s, string data) {
	/**
	* The sent packet consist of a header and a data section: [message] = [header(16byte)||data]
	* the header size is HEADER_SIZE bytes, contains the data length.
	*/

	// Create Header
	string header;
	int dataLen = data.size();
	int dataLen_size = (int)log10(dataLen) + 1;
	header = to_string(dataLen);
	// We will add " " into the header to make sure that the length of header is HEADER_SIZE bytes
	for (int i = 0; i < HEADER_SIZE - dataLen_size; i++) {
		header.append(" ");
	}

	// Create Packet
	string packet;
	packet.append(header);
	packet.append(data);

	// Send Packet
	int ret = send(s, packet.c_str(), packet.size(), 0);

	return ret;

}

/**
* Function: recv_finetune
* Description:	an improved version of recv function, which helps decrypting
*				the message was sent by send_finetune function.
* Param		s		The descriptor that identifies a connected socket
* Param		data	the buffer which receive the incoming data.
*/
int recv_finetune(SOCKET s, string &message) {
	message = "";
	// reading packet header to get the data size
	char dataLen_buff[HEADER_SIZE + 1];
	int ret = recv(s, dataLen_buff, HEADER_SIZE, MSG_WAITALL);

	if (ret > 0) {
		dataLen_buff[ret] = 0;
		int dataLen = atoi(dataLen_buff);

		// reading packet data
		if (dataLen > 0) {
			char *buff = new char[dataLen + 16];
			ret = recv(s, buff, dataLen, 0);
			if (ret > 0) {
				buff[ret] = 0;
				message.append(buff);
			}
		}
	}

	return ret;
}

/**
 * Function: ProcessQuestion
 * Description: split the input string into 2 strings, one contain only alphabets,
 *				the other string contain only digit. If the input string has non-alphabet 
 *				or non-digit character, an error message will be returned 
 * Param		str		the input string
 * Return	A pointer to a buffer containing a list of alphabets and a list of digits, separated by " "
 */
char *ProcessQuestion(string clientRequest) {
	int BUFF_SIZE = clientRequest.size() + 16;
	char *str = (char*)clientRequest.c_str();
	char *buff = new char[BUFF_SIZE];
	char *alphabet = new char[BUFF_SIZE];
	char *digit = new char[BUFF_SIZE];
	strcpy_s(buff, BUFF_SIZE, "");
	strcpy_s(alphabet, BUFF_SIZE, "");
	strcpy_s(digit, BUFF_SIZE, "");

	for (int i = 0; str[i] != NULL; i++) {
		if (isalpha((unsigned char) str[i])) strncat_s(alphabet, BUFF_SIZE, &str[i], 1);
		else if (isdigit((unsigned char) str[i])) strncat_s(digit, BUFF_SIZE, &str[i], 1);
		else {
			strcat_s(buff, BUFF_SIZE, "-"); // Add prefix "-" in message to help client realize the error message
			return buff;
		}
	}
	strcat_s(buff, BUFF_SIZE, "+"); // Add prefix "+" in message to help client realize the message contain result 
	strcat_s(buff, BUFF_SIZE, alphabet);
	strcat_s(buff, BUFF_SIZE, " ");
	strcat_s(buff, BUFF_SIZE, digit);
	return buff;
}


int main(int argc, char *argv[]) {
	// Step 1: Initiate WinSock
	WSAData wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("WinSock 2.2 is not supported\n");
		return 0;
	}

	// Step 2: Construct socket
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSock == INVALID_SOCKET) {
		printf("[Error %d] Cannot create TCP socket.\n", WSAGetLastError());
		return 0;
	}

	// Step 3: Bind address to socket
	const int SERVER_PORT = atoi(argv[1]);
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr))) {
		printf("(Error: %d) Cannot associate a local address with server socket\n", WSAGetLastError());
		return 0;
	}

	// Step 4: Listen request from client
	if (listen(listenSock, MAX_CLIENT)) {
		printf("(Error: %d) Cannot place server socket in state LISTEN\n", WSAGetLastError());
		return 0;
	}

	printf("Server started!\n");

	// Step 5: Communicate with client
	SOCKADDR_IN clientAddr;

	char clientIP[INET_ADDRSTRLEN];
	string clientRequest, serverAnswer;
	int ret, clientAddrLen = sizeof(clientAddr), clientPort;
	SOCKET connSock;

	while (1) {
		// accept request
		connSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);
		if (connSock == SOCKET_ERROR) {
			printf("[Error %d] Cannot permit incoming connection", WSAGetLastError());
			continue;
		}
		else {
			inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
			clientPort = ntohs(clientAddr.sin_port);
			printf("Accept incoming connection from %s:%d\n", clientIP, clientPort);
		} 

		while (1) {
			// Receive message from client
			ret = recv_finetune(connSock, clientRequest);

			if (ret == SOCKET_ERROR) {
				printf("Error %d: Cannot receiv data!\n", WSAGetLastError());
				break;
			} 
			else if (ret == 0) {
				printf("Client disconnected\n");
				break;
			}
			else {
				cout << "Receive from client [" << clientIP << ":" << clientPort << "]: " << clientRequest << endl;
				string serverAnswer(ProcessQuestion(clientRequest)); // Processing request
				ret = send_finetune(connSock, serverAnswer);

				if (ret == SOCKET_ERROR) {
					printf("Error %d: Cannot send data.\n", WSAGetLastError());
					break;
				}
			}
		} //End communicating with 1 client
		closesocket(connSock);
	}
	closesocket(listenSock);

	WSACleanup();

	return 0;
}