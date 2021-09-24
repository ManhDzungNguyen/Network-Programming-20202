#include "stdafx.h"
#include <stdio.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <vector>
#include <iostream>
using namespace std;

#define HEADER_SIZE 10 // size of header of a TCP packet, 10: the max number of digits of an "int" number.
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
	 * The sent packet consist of a header and a data section
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
			delete[] buff;
		}
	}

	return ret;
}

/**
 * Function: decryptServerMesssage
 * Description: decrypt the Server response and show the result
 * Param		buff	A pointer to a buffer containing the Server message
 */
void decryptServerMesssage(char* buff) {
	// Check the message from server is an error message or not
	if (buff[0] == '-')
		printf("Error\n");
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
		for (size_t n = 0; n < v.size(); n++)
			printf("%s\n", v[n]);
	}
	else
		printf("Can not read the message from server!\n");
	printf("\n");
}

int main(int argc, char* argv[]) {
	// Step 1: Initiate WinSock
	WSAData wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("WinSock 2.2 is not supported\n");
		return 0;
	}

	// Step 2: Construct socket
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client == INVALID_SOCKET) {
		printf("Error %d: Cannot create TCP socket! ", WSAGetLastError());
		return 0;
	}

	// Step 3: Specify server address
	const char* SERVER_ADDR = argv[1];
	const int SERVER_PORT = atoi(argv[2]);
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	// Step 4: Request to connect server
	if (connect(client, (sockaddr*)&serverAddr, sizeof(serverAddr))) {
		printf("Error %d: Cannot connect to server", WSAGetLastError());
		return 0;
	}
	printf("Connected to server!\n");

	// Step 5: Communicate with server
	string userInput, serverAnswer;
	int ret, messageLen;
	while (1) {
		// Send message
		printf("Send to server: ");
		fflush(stdin);
		getline(cin, userInput);
		messageLen = userInput.size();
		if (messageLen == 0)
			break;

		ret = send_finetune(client, userInput);
		if (ret == SOCKET_ERROR)
			printf("Error %d: Cannot send data\n", WSAGetLastError());

		// Receive echo message
		ret = recv_finetune(client, serverAnswer);
		if (ret == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) {
				printf("Time-out!\n");
			}
			else
				printf("[Error %d] Cannot receive data\n", WSAGetLastError());
		}
		else if (serverAnswer.size() > 0){
			decryptServerMesssage((char*)serverAnswer.c_str()); // decrypt Server Message and show Output
		}
	}

	// Step 6: Close socket
	closesocket(client);

	// Step 7: Terminate Winsock
	WSACleanup();

	return 0;
}
