// WSAEventSelectServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <conio.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <ctime>
#include <fstream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")

#define PORT 6000
#define BUFF_SIZE 2048
#define SERVER_ADDR "127.0.0.1"
#define TRANSID_SIZE 10 
#define DATALEN_SIZE 10 
#define ACCOUNT_FILE "account.txt"
#define LOG_FILE "log_20183899.txt"


typedef struct client_session_management {
	bool free;
	SOCKET socket;
	int clientPort;
	char* clientIP;
	string username;
	bool isLogin;
} CLIENT;


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
	string transID_buff = "";
	char transID_buff_part[TRANSID_SIZE + 1];
	int dataLen = TRANSID_SIZE, ret;
	while (dataLen > 0) {
		ret = recv(s, transID_buff_part, dataLen, 0);
		if (ret < 0) return ret;
		if (ret > 0) {
			dataLen -= ret;
			transID_buff_part[ret] = 0;
			transID_buff.append(transID_buff_part);
		}
	}
	transID = atoi(transID_buff.c_str());

	// reading packet dataLen_block
	string dataLen_buff = "";
	char dataLen_buff_part[DATALEN_SIZE + 1];
	dataLen = DATALEN_SIZE;
	while (dataLen > 0) {
		ret = recv(s, dataLen_buff_part, dataLen, 0);
		if (ret < 0) return ret;
		if (ret > 0) {
			dataLen -= ret;
			dataLen_buff_part[ret] = 0;
			dataLen_buff.append(dataLen_buff_part);
		}
	}

	dataLen = atoi(dataLen_buff.c_str());
	int result = dataLen;

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
	}

	return result;
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
int login(CLIENT* client, string username) {
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
int post_message(CLIENT* client) {
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
int logout(CLIENT* client) {
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
int ProcessRequest(string clientRequest, CLIENT* client) {
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
void log_record(CLIENT client, string message, int result) {
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


int main(int argc, char* argv[])
{
	DWORD		nEvents = 0;
	DWORD		index;
	//SOCKET		socks[WSA_MAXIMUM_WAIT_EVENTS];
	CLIENT		client[WSA_MAXIMUM_WAIT_EVENTS];
	WSAEVENT	events[WSA_MAXIMUM_WAIT_EVENTS];
	WSANETWORKEVENTS sockEvent;

	//Step 1: Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Winsock 2.2 is not supported\n");
		return 0;
	}

	//Step 2: Construct LISTEN socket	
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Step 3: Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	client[0].socket = listenSock;
	events[0] = WSACreateEvent(); //create new events
	nEvents++;

	// Associate event types FD_ACCEPT and FD_CLOSE
	// with the listening socket and newEvent   
	WSAEventSelect(client[0].socket, events[0], FD_ACCEPT | FD_CLOSE);


	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr)))
	{
		printf("Error %d: Cannot associate a local address with server socket.", WSAGetLastError());
		return 0;
	}

	//Step 4: Listen request from client
	if (listen(listenSock, 10)) {
		printf("Error %d: Cannot place server socket in state LISTEN.", WSAGetLastError());
		return 0;
	}

	printf("Server started!\n");

	SOCKET connSock;
	sockaddr_in clientAddr;
	char clientIP[INET_ADDRSTRLEN];
	int clientAddrLen = sizeof(clientAddr);
	int ret, i, clientPort;

	for (i = 1; i < WSA_MAXIMUM_WAIT_EVENTS; i++) {
		client[i] = { true, NULL, 0, "", "", false };
	}
	while (1) {
		//wait for network events on all socket
		index = WSAWaitForMultipleEvents(nEvents, events, FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_FAILED) {
			printf("Error %d: WSAWaitForMultipleEvents() failed\n", WSAGetLastError());
			break;
		}

		index = index - WSA_WAIT_EVENT_0;
		WSAEnumNetworkEvents(client[index].socket, events[index], &sockEvent);

		if (sockEvent.lNetworkEvents & FD_ACCEPT) {
			if (sockEvent.iErrorCode[FD_ACCEPT_BIT] != 0) {
				printf("FD_ACCEPT failed with error %d\n", sockEvent.iErrorCode[FD_READ_BIT]);
				continue;
			}

			if ((connSock = accept(client[index].socket, (sockaddr *)&clientAddr, &clientAddrLen)) == SOCKET_ERROR) {
				printf("Error %d: Cannot permit incoming connection.\n", WSAGetLastError());
				continue;
			}

			//Add new socket into socks array
			int i;
			if (nEvents == WSA_MAXIMUM_WAIT_EVENTS) {
				printf("\nToo many clients.");
				closesocket(connSock);
			}
			else {
				inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
				clientPort = ntohs(clientAddr.sin_port);

				for (i = 1; i < WSA_MAXIMUM_WAIT_EVENTS; i++)
					if (client[i].free) {
						client[i] = { false, connSock, clientPort, clientIP, "", false };
						events[i] = WSACreateEvent();
						WSAEventSelect(client[i].socket, events[i], FD_READ | FD_CLOSE);
						nEvents++;
						break;
					}
			}

			//reset event
			WSAResetEvent(events[index]);
		}

		if (sockEvent.lNetworkEvents & FD_READ) {
			//Receive message from client
			if (sockEvent.iErrorCode[FD_READ_BIT] != 0) {
				printf("FD_READ failed with error %d\n", sockEvent.iErrorCode[FD_READ_BIT]);
				continue;
			}
			int requestID;
			string buff;
			ret = recv_finetune_server(client[index].socket, requestID, buff);

			//Release socket and event if an error occurs
			if (ret <= 0) {
				if (index == nEvents - 1) {
					closesocket(client[index].socket);
					client[index].free = true;
					client[index].username = "";
					WSACloseEvent(events[index]);
				}
				else {
					closesocket(client[index].socket);
					client[index] = client[nEvents - 1];
					WSACloseEvent(events[index]);
					events[index] = WSACreateEvent();
					WSAEventSelect(client[index].socket, events[index], FD_READ | FD_CLOSE);
					client[nEvents - 1].free = true;
					client[nEvents - 1].username = "";
					WSACloseEvent(events[nEvents - 1]);
				}
				nEvents--;
			}
			else {			
				int result;
				cout << "Receive from client [" << clientIP << ":" << clientPort << "]: " << buff << endl;

				result = ProcessRequest(buff, &client[index]); // Processing request

				// Send request processing result code
				string serverAnswer = to_string(result);
				ret = send_finetune_server(client[index].socket, requestID, serverAnswer);
				if (ret == SOCKET_ERROR)
					printf("Error %d: Cannot send data.\n", WSAGetLastError());

				log_record(client[index], buff, result); // Record detail to log record file

				//reset event
				WSAResetEvent(events[index]);
			}
		}

		if (sockEvent.lNetworkEvents & FD_CLOSE) {
			if (sockEvent.iErrorCode[FD_CLOSE_BIT] != 0) {
				printf("FD_CLOSE failed with error %d\n", sockEvent.iErrorCode[FD_CLOSE_BIT]);
				continue;
			}
			//Release socket and event
			if (index == nEvents - 1) {
				closesocket(client[index].socket);
				client[index].free = true;
				client[index].username = "";
				WSACloseEvent(events[index]);
			}
			else {
				closesocket(client[index].socket);
				client[index] = client[nEvents - 1];
				WSACloseEvent(events[index]);
				events[index] = WSACreateEvent();
				WSAEventSelect(client[index].socket, events[index], FD_READ | FD_CLOSE);
				client[nEvents - 1].free = true;
				client[nEvents - 1].username = "";
				WSACloseEvent(events[nEvents - 1]);
			}
			nEvents--;
		}
	}
	return 0;
}
