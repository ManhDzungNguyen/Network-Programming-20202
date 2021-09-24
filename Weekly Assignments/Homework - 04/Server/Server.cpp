#ifndef FD_SETSIZE
#define FD_SETSIZE 1024
#endif
#include "stdafx.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include "process.h"
#include <sstream>
#include <ctime>
#include <fstream>
using namespace std;

#define SERVER_ADDR "127.0.0.1"
#define MAX_CLIENT 10
#define NUM_THREAD 10
#define TRANSID_SIZE 10 
#define DATALEN_SIZE 10 
#define ACCOUNT_FILE "account.txt"
#define LOG_FILE "log_20183899.txt"
#define BUFF_SIZE 2048
#define MESSAGE_FORMAT_ERROR -2

#pragma comment(lib, "Ws2_32.lib")

typedef struct client_session_management {
	bool free;
	SOCKET socket;
	int clientPort;
	char* clientIP;
	string username;
	bool isLogin;
} c_ses;


/**
* Function:	send_finetune_server
* Description: an improved version of send function, fine-tuned to work with server only.
* Param	s		A descriptor identifying a connected socket
* Param	transID		transaction ID of client message which will be replied now
* Param	data	the data to be transmitted
*/
int send_finetune_server(SOCKET s, int transID, string data) {
	/**
	* The sent packet consist of a transaction ID section, a Data length section and a data section:
	* [message] = [transID_block(10 bytes) || dataLen_block(10 bytes) || data]
	*/

	// Create transaction ID field
	string transID_block;
	int transID_size = (int)log10(transID) + 1;
	transID_block = to_string(transID);
	// Padding to the last of transID_block
	for (int i = 0; i < TRANSID_SIZE - transID_size; i++) {
		transID_block.append(" ");
	}

	// Create Data length field
	data.append("\r\n"); // add a special symbol to mark EOF

	string dataLen_block;
	int dataLen = data.size();
	int dataLen_size = (int)log10(dataLen) + 1;
	dataLen_block.append(to_string(dataLen));
	// Padding to the last of dataLen_block
	for (int i = 0; i < DATALEN_SIZE - dataLen_size; i++) {
		dataLen_block.append(" ");
	}

	// Create Packet
	string packet;
	packet.append(transID_block);
	packet.append(dataLen_block);
	packet.append(data);
	char* buff = (char*)packet.c_str();
	int size = packet.size();

	// Send Packet
	int idx = 0;
	int nLeft = size;
	int ret = -1;
	while (nLeft > 0) {
		int ret = send(s, buff + idx, nLeft, 0);
		if (ret == SOCKET_ERROR)
			return ret;
		nLeft -= ret;
		idx += ret;
	}


	return size;

}


/**
* Function:	recv_finetune_client
* Description: an improved version of recv function, fine-tuned to work with server only.
* Param	s		A descriptor identifying a connected socket
* Param	transID		transaction ID of received message
* Param	buff	the buffer which receive the incoming data.
* Return		the data to be received
*/
int recv_finetune_server(SOCKET s, int& transID, string &buff) {
	buff = "";
	// reading packet transID_block 
	char transID_buff[TRANSID_SIZE + 1];
	int dataLen = 0;
	int ret = recv(s, transID_buff, TRANSID_SIZE, MSG_WAITALL);
	if(ret <= 0) return ret;

	transID_buff[ret] = 0;
	transID = atoi(transID_buff);

	// reading packet dataLen_block
	char dataLen_buff[DATALEN_SIZE + 1];
	ret = recv(s, dataLen_buff, DATALEN_SIZE, MSG_WAITALL);
	if (ret <= 0) return ret;

	dataLen_buff[ret] = 0;
	dataLen = atoi(dataLen_buff);

	// reading packet data
	if (dataLen > 0) {
		char *buff_part = new char[dataLen + 16];
		while (dataLen > 0) {
			ret = recv(s, buff_part, dataLen, 0);
			if (ret > 0) {
				dataLen -= ret;
				buff_part[ret] = 0;
				buff.append(buff_part);
			}
		}

		string endRequest = buff.substr(buff.size() - 2, 2);
		if (endRequest.compare("\r\n") != 0)
			return MESSAGE_FORMAT_ERROR;
		else {
			buff = buff.substr(0, buff.size() - 2);
			return buff.size();
		}
	}

	return dataLen;
}


/**
* Function:	split
* Description:	split the input string into substrings based on the input delimiter.
* Param	str		the input string
* Param	token	the delimiter
* Return		A vector contains list of substrings
*/
vector<string> split(string str, string token) {
	vector<string>result;
	while (str.size()) {
		int index = str.find(token);
		if (index != string::npos) {
			result.push_back(str.substr(0, index));
			str = str.substr(index + token.size());
			if (str.size() == 0)result.push_back(str);
		}
		else {
			result.push_back(str);
			str = "";
		}
	}
	return result;
}


/**
* Function:	getTime
* Description:	get the current time.
* Return		A string which shows the current time in a specific form
*/
const string getTime() {
	time_t     now = time(0);
	struct tm  newtime;
	char       buf[80];
	localtime_s(&newtime, &now);
	// More information about date/time format: http://en.cppreference.com/w/cpp/chrono/c/strftime
	strftime(buf, sizeof(buf), "[%d/%m/%Y %X]", &newtime);

	return buf;
}


/**
* Function:	login
* Description: process login request from client
* Param	client		the client information and status
* Param	username	username request from client
* Return		result code
*/
int login(c_ses* client, string username) {
	if (client->isLogin)
		return 13;

	string availableStatus = "0";

	ifstream file(ACCOUNT_FILE);
	string str;
	while (getline(file, str)) {
		vector<string> acc = split(str, " ");
		if (username.compare(acc[0]) == 0) {
			if (availableStatus.compare(acc[1]) == 0) {
				client->isLogin = true;
				client->username = username;
				return 10;
			}
			return 12;
		}
	}

	return 11;

}


/**
* Function:	post_message
* Description: process post message request from client,
allow user to post message after login successfully.
* Param	client		the client information and status
* Return		result code
*/
int post_message(c_ses* client) {
	if (!(client->isLogin))
		return 21;

	return 20;
}


/**
* Function:	logout
* Description: process log out request from client,
allow user to post message after login successfully.
* Param	client		the client information and status
* Return		result code
*/
int logout(c_ses* client) {
	if (!(client->isLogin))
		return 31;
	client->isLogin = false;
	client->username = "";
	return 30;
}


/**
* Function:	ProcessRequest
* Description:	process client request
* Param	clientRequest	the client request
* Param	client		the client information and status
* Return		result code
*/
int ProcessRequest(string clientRequest, c_ses* client) {
	int result_code;
	vector<string> token = split(clientRequest, " ");
	string mode = token[0];

	if (mode.compare("USER") == 0) {
		string username = token[1];
		result_code = login(client, username);
	}
	else if (mode.compare("POST") == 0) {
		string message = token[1];
		result_code = post_message(client);
	}
	else if (mode.compare("EXIT") == 0) {
		result_code = logout(client);
	}
	else
		result_code = 40;

	return result_code;
}


/**
* Function:	log_record
* Description:	Record information to log record file
* Param	client		the client information and status
* Param	message		client request
* Param	result		result code of client request
*/
void log_record(c_ses client, string message, int result) {
	int clientPort = client.clientPort;
	char* clientIP = client.clientIP;
	ostringstream stringStream;
	stringStream << clientIP << ":" << clientPort << " " << getTime() << " $ " << message << " $ " << result;

	string log_message = stringStream.str();

	ofstream logFile;
	logFile.open(LOG_FILE, std::ios::app);
	logFile << log_message << endl;
	logFile.close();
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
	//const int SERVER_PORT = 5500;

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


	SOCKET connSock;
	char clientIP[INET_ADDRSTRLEN];
	c_ses client[FD_SETSIZE];
	fd_set readfds, initfds; //use initfds to initiate readfds at the begining of every loop step
	sockaddr_in clientAddr;
	int nEvents, clientAddrLen, clientPort;

	for (int i = 0; i < FD_SETSIZE; i++)
		client[i] = { true, NULL, 0, "", "", false };	// 0 indicates available entry

	FD_ZERO(&initfds);
	FD_SET(listenSock, &initfds);


	// Step 5: Communicate with client
	while (1) {
		readfds = initfds;		/* structure assignment */
		nEvents = select(0, &readfds, 0, 0, 0);
		if (nEvents < 0) {
			printf("\nError! Cannot poll sockets: %d", WSAGetLastError());
			break;
		}

		//new client connection
		if (FD_ISSET(listenSock, &readfds)) {
			clientAddrLen = sizeof(clientAddr);
			if ((connSock = accept(listenSock, (sockaddr *)&clientAddr, &clientAddrLen)) < 0) {
				printf("\nError! Cannot accept new connection: %d", WSAGetLastError());
				break;
			}
			else {
				inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
				clientPort = ntohs(clientAddr.sin_port);
				printf("You got a connection from %s\n", clientIP); /* prints client's IP */

				int i;
				for (i = 0; i < FD_SETSIZE; i++)
					if (client[i].free) {
						client[i] = { false, connSock, clientPort, clientIP, "", false };
						FD_SET(client[i].socket, &initfds);
						break;
					}

				if (i == FD_SETSIZE) {
					printf("\nToo many clients.");
					closesocket(connSock);
				}

				if (--nEvents == 0)
					continue; //no more event
			}
		}

		//receive data from clients
		for (int i = 0; i < FD_SETSIZE; i++) {
			if (client[i].free)
				continue;

			string buff;
			int requestID;

			if (FD_ISSET(client[i].socket, &readfds)) {
				int ret = recv_finetune_server(client[i].socket, requestID, buff);
				if (ret == SOCKET_ERROR) {
					printf("Error %d: Cannot receive data.\n", WSAGetLastError());
					client[i].free = true;
					client[i].username = "";
					FD_CLR(client[i].socket, &initfds);
					closesocket(client[i].socket);

				}
				else if (ret == MESSAGE_FORMAT_ERROR) {
					cout << "Error: Incorrect mesage format found!" << endl;
				}
				else if (ret == 0) {
					printf("Client disconnects.\n");
					client[i].free = true;
					client[i].username = "";
					FD_CLR(client[i].socket, &initfds);
					closesocket(client[i].socket);
				}
				else if (buff.size() > 0) {
					int result;
					cout << "Receive from client [" << clientIP << ":" << clientPort << "]: " << buff << endl;

					result = ProcessRequest(buff, &client[i]); // Processing request

					// Send request processing result code
					string serverAnswer = to_string(result);
					ret = send_finetune_server(client[i].socket, requestID, serverAnswer);
					if (ret == SOCKET_ERROR)
						printf("Error %d: Cannot send data.\n", WSAGetLastError());

					log_record(client[i], buff, result); // Record detail to log record file
				}
			}

			if (--nEvents <= 0)
				continue; //no more event
		}

	}



	closesocket(listenSock);

	WSACleanup();

	return 0;
}
