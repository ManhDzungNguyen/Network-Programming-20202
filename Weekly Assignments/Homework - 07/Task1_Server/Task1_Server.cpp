// OEBServer.cpp : This sample illustrates how to develop a simple echo server Winsock
// application using the Overlapped I/O model with event notification.
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

#define SERVER_ADDR "127.0.0.1"
#define PORT 6000
#define DATA_BUFSIZE 8192
#define TRANSID_SIZE 10 
#define DATALEN_SIZE 10
#define ACCOUNT_FILE "account.txt"
#define LOG_FILE "log_20183899.txt"
#define RECEIVE 0
#define SEND 1
#pragma comment(lib, "Ws2_32.lib")

/*Struct contains information of the socket communicating with client*/
typedef struct client_session_management {
	SOCKET socket;
	WSAOVERLAPPED overlapped;
	WSABUF dataBuf;
	char buffer[DATA_BUFSIZE];
	int clientPort;
	char* clientIP;

	char username[DATA_BUFSIZE];
	bool isLogin;
	int operation;

	int recvTransIDLeft;
	char transID_block[DATA_BUFSIZE];
	int transID;

	int recvDataLenLeft;
	char dataLen_block[DATA_BUFSIZE];

	int recvDataLeft;
	char data_block[DATA_BUFSIZE];

	int messageLen;
	int sentBytes;
} CLIENT;

/**
* Function:	isInt
* Description:	check a char array is unsigned interger or not
* Param	array		the array needed to check
* Param	size		size of array
* Return		check result
*/
bool isInt(char* array, int size) {
	for (int i = 0; i < size; i++) {
		if (!(array[i] == '1' || array[i] == '2' || array[i] == '3' ||
			array[i] == '4' || array[i] == '5' || array[i] == '6' ||
			array[i] == '7' || array[i] == '8' || array[i] == '9' ||
			array[i] == '0' || array[i] == ' ')) {
			return false;
		}
	}
	return true;
}

/**
* Function:	createSendMessage
* Description:	create a return message for client, save in buffer of client
* Param	client		the client information and status
* Param	result		the result from server to be transmitted
* Return		Message size
*/
int createSendMessage(CLIENT* client, string result) {
	/**
	* The sent packet consist of a transaction ID section, a Data length section and a data section:
	* [message] = [transID_block(10 bytes) || dataLen_block(10 bytes) || data]
	*/

	// Create transaction ID field
	string transID_block;
	int transID_size = (int)log10(client->transID) + 1;
	transID_block = to_string(client->transID);
	// Padding to the last of transID_block
	for (int i = 0; i < TRANSID_SIZE - transID_size; i++) {
		transID_block.append(" ");
	}

	// Create Data length field
	string dataLen_block;
	int dataLen = result.size();
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
	packet.append(result);
	int size = packet.size();
	strcpy_s(client->buffer, DATA_BUFSIZE, packet.c_str());

	return size;
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
				strcpy_s(client->username, DATA_BUFSIZE, username.c_str());
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
	strcpy_s(client->username, DATA_BUFSIZE, "");

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
* Param	result		result code of client request
*/
void log_record(CLIENT* client, int result) {
	ostringstream stringStream;
	stringStream << client->clientIP << ":" << client->clientPort << " " << getTime() << " $ " << client->data_block << " $ " << result;

	string log_message = stringStream.str();

	ofstream logFile;
	logFile.open(LOG_FILE, std::ios::app);
	logFile << log_message << endl;
	logFile.close();
}


/**
* The freeClientInfo function remove a client management from array
* @param	siArray		An array of pointers of socket information struct
* @param	n	Index of the removed socket
*/

void freeClientInfo(CLIENT* siArray[], int n) {
	closesocket(siArray[n]->socket);
	free(siArray[n]);
	siArray[n] = 0;
	for (int i = n; i < WSA_MAXIMUM_WAIT_EVENTS - 1; i++) {
		siArray[i] = siArray[i + 1];
	}
}

/**
* The closeEventInArray function release an event and remove it from an array
* @param	eventArr	An array of event object handles
* @param	n	Index of the removed event object
*/
void closeEventInArray(WSAEVENT eventArr[], int n) {
	WSACloseEvent(eventArr[n]);

	for (int i = n; i < WSA_MAXIMUM_WAIT_EVENTS - 1; i++)
		eventArr[i] = eventArr[i + 1];
}

int main()
{
	CLIENT* clients[WSA_MAXIMUM_WAIT_EVENTS];
	WSAEVENT events[WSA_MAXIMUM_WAIT_EVENTS];
	int nEvents = 0;

	WSADATA wsaData;
	if (WSAStartup((2, 2), &wsaData) != 0) {
		printf("WSAStartup() failed with error %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	for (int i = 0; i < WSA_MAXIMUM_WAIT_EVENTS; i++) {
		clients[i] = 0;
		events[i] = 0;
	}

	// Start Winsock and set up a LISTEN socket
	SOCKET listenSocket;
	if ((listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket.", WSAGetLastError());
		return 1;
	}

	// LISTEN socket associate with an event object 
	events[0] = WSACreateEvent();
	nEvents++;
	WSAEventSelect(listenSocket, events[0], FD_ACCEPT | FD_CLOSE);

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	if (bind(listenSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)))
	{
		printf("Error %d: Cannot associate a local address with server socket.", WSAGetLastError());
		return 0;
	}

	if (listen(listenSocket, 10)) {
		printf("Error %d: Cannot place server socket in state LISTEN.", WSAGetLastError());
		return 0;
	}

	printf("Server started!\n");
	
	int index;
	SOCKET connSock;
	sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);

	while (1) {
		// Wait for network events on all socket
		index = WSAWaitForMultipleEvents(nEvents, events, FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_FAILED) {
			printf("Error %d: WSAWaitForMultipleEvents() failed\n", WSAGetLastError());
			continue;
		}
		index = index - WSA_WAIT_EVENT_0;
		DWORD flags, transferredBytes;

		// If the event triggered was zero then a connection attempt was made

		// on our listening socket.
		if (index == 0) {
			WSAResetEvent(events[0]);
			if ((connSock = accept(listenSocket, (sockaddr *)&clientAddr, &clientAddrLen)) == INVALID_SOCKET) {
				printf("Error %d: Cannot permit incoming connection.\n", WSAGetLastError());
				continue;
			}
			int i;
			if (nEvents == WSA_MAXIMUM_WAIT_EVENTS) {
				printf("\nToo many clients.");
				closesocket(connSock);
			}
			else {
				// Disassociate connected socket with any event object
				WSAEventSelect(connSock, NULL, 0);

				// Append connected socket to the array of SocketInfo
				i = nEvents;
				events[i] = WSACreateEvent();
				clients[i] = (CLIENT *)malloc(sizeof(CLIENT));
				clients[i]->socket = connSock;
				memset(&clients[i]->overlapped, 0, sizeof(WSAOVERLAPPED));
				clients[i]->overlapped.hEvent = events[i];
				clients[i]->dataBuf.buf = clients[i]->buffer;
				clients[i]->dataBuf.len = TRANSID_SIZE;
				char clientIP[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
				clients[i]->clientIP = clientIP;
				clients[i]->clientPort = ntohs(clientAddr.sin_port);
				
				strcpy_s(clients[i]->username, DATA_BUFSIZE, "");
				clients[i]->operation = RECEIVE;
				clients[i]->recvTransIDLeft = TRANSID_SIZE;
				clients[i]->recvDataLenLeft = DATALEN_SIZE;
				strcpy_s(clients[i]->transID_block, DATA_BUFSIZE, "");
				strcpy_s(clients[i]->dataLen_block, DATA_BUFSIZE, "");
				strcpy_s(clients[i]->data_block, DATA_BUFSIZE, "");
				clients[i]->sentBytes = 0;
				clients[i]->isLogin = false;

				nEvents++;
				
				// Post an overlpped I/O request to begin receiving data on the socket
				flags = 0;
				if (WSARecv(clients[i]->socket, &(clients[i]->dataBuf), 1, &transferredBytes, &flags, &(clients[i]->overlapped), NULL) == SOCKET_ERROR) {
					if (WSAGetLastError() != WSA_IO_PENDING) {
						printf("WSARecv() failed with error %d\n", WSAGetLastError());
						closeEventInArray(events, i);
						freeClientInfo(clients, i);
						nEvents--;
					}
				}
			}
		}
		else { // If the event triggered wasn't zero then an I/O request is completed.
			CLIENT *client;
			client = clients[index];
			WSAResetEvent(events[index]);
			BOOL result;
			result = WSAGetOverlappedResult(client->socket, &(client->overlapped), &transferredBytes, FALSE, &flags);
			if (result == FALSE || transferredBytes == 0) {
				closesocket(client->socket);
				closeEventInArray(events, index);
				freeClientInfo(clients, index);
				client = 0;
				nEvents--;
				continue;
			}

			// Check to see if the operation field equals RECEIVE. If this is so, then
			// this means a WSARecv call just completed
			if (client->operation == RECEIVE) {
				client->buffer[transferredBytes] = 0;
				/*
				* check the server receive all transID block or not
				* if all transID block is received, client->recvTransIDLeft will equal to 0
				*/
				if (client->recvTransIDLeft > 0) { 
					strcat_s(client->transID_block, DATA_BUFSIZE, client->buffer);
					int transIDLeft = client->recvTransIDLeft - transferredBytes;
					client->recvTransIDLeft = client->recvTransIDLeft - transferredBytes;
					if (client->recvTransIDLeft > 0) {
						flags = 0;
						client->operation = RECEIVE;
						client->dataBuf.len = client->recvTransIDLeft;

						if (WSARecv(client->socket, &(client->dataBuf), 1, &transferredBytes, &flags, &(client->overlapped), NULL) == SOCKET_ERROR) {
							if (WSAGetLastError() != WSA_IO_PENDING) {
								printf("WSARecv() failed with error %d\n", WSAGetLastError());
								closeEventInArray(events, index);
								freeClientInfo(clients, index);
								nEvents--;
								continue;
							}
						}
						
					}
					else {
						if(isInt(client->transID_block, TRANSID_SIZE))
							client->transID = atoi(client->transID_block);
						else {
							printf("Wrong Message Format at transID_block\n");
							closeEventInArray(events, index);
							freeClientInfo(clients, index);
							nEvents--;
							continue;
						}

						flags = 0;
						client->operation = RECEIVE;
						client->dataBuf.len = DATALEN_SIZE;

						if (WSARecv(client->socket, &(client->dataBuf), 1, &transferredBytes, &flags, &(client->overlapped), NULL) == SOCKET_ERROR) {
							if (WSAGetLastError() != WSA_IO_PENDING) {
								printf("WSARecv() failed with error %d\n", WSAGetLastError());
								closeEventInArray(events, index);
								freeClientInfo(clients, index);
								nEvents--;
								continue;
							}
						}

					}
				}

				/*
				* check the server receive all dataLen block or not
				* if all dataLen block is received, client->recvDataLenLeft will equal to 0
				*/
				else if (client->recvDataLenLeft > 0) {
					strcat_s(client->dataLen_block, DATA_BUFSIZE, client->buffer);
					client->recvDataLenLeft = client->recvDataLenLeft - transferredBytes;

					if (client->recvDataLenLeft > 0) {
						flags = 0;
						client->operation = RECEIVE;
						client->dataBuf.len = client->recvDataLenLeft;

						if (WSARecv(client->socket, &(client->dataBuf), 1, &transferredBytes, &flags, &(client->overlapped), NULL) == SOCKET_ERROR) {
							if (WSAGetLastError() != WSA_IO_PENDING) {
								printf("WSARecv() failed with error %d\n", WSAGetLastError());
								closeEventInArray(events, index);
								freeClientInfo(clients, index);
								nEvents--;
								continue;
							}
						} 

					}
					else {
						if (isInt(client->dataLen_block, DATALEN_SIZE))
							client->recvDataLeft = atoi(client->dataLen_block);
						else {
							printf("Wrong Message Format at dataLen_block\n");
							closeEventInArray(events, index);
							freeClientInfo(clients, index);
							nEvents--;
							continue;
						}
						flags = 0;
						client->operation = RECEIVE;
						if (client->recvDataLeft > DATA_BUFSIZE - 1)
							client->dataBuf.len = DATA_BUFSIZE - 1;
						else
							client->dataBuf.len = client->recvDataLeft;

						if (client->recvDataLeft > 0) {
							if (WSARecv(client->socket, &(client->dataBuf), 1, &transferredBytes, &flags, &(client->overlapped), NULL) == SOCKET_ERROR) {
								if (WSAGetLastError() != WSA_IO_PENDING) {
									printf("WSARecv() failed with error %d\n", WSAGetLastError());
									closeEventInArray(events, index);
									freeClientInfo(clients, index);
									nEvents--;
									continue;
								}
							}
						}
						else {
							cout << "Client [" << client->clientIP << ":" << client->clientPort << "] Disconnected! "<< endl;
							closeEventInArray(events, index);
							freeClientInfo(clients, index);
							nEvents--;
							continue;
						}

					}
				}

				/*
				* check the server receive all dât block or not
				* if all data block is received, client->recvDataLeft will equal to 0
				*/
				else if (client->recvDataLeft > 0) {
					strcat_s(client->data_block, DATA_BUFSIZE, client->buffer);
					client->recvDataLeft = client->recvDataLeft - transferredBytes;

					if (client->recvDataLeft > 0) {
						flags = 0;
						client->operation = RECEIVE;
						client->dataBuf.len = client->recvDataLeft;

						if (WSARecv(client->socket, &(client->dataBuf), 1, &transferredBytes, &flags, &(client->overlapped), NULL) == SOCKET_ERROR) {
							if (WSAGetLastError() != WSA_IO_PENDING) {
								printf("WSARecv() failed with error %d\n", WSAGetLastError());
								closeEventInArray(events, index);
								freeClientInfo(clients, index);
								nEvents--;
								continue;
							}
						}

					}
					// if operation is SEND
					else {
						string data = "";
						data.append(client->data_block);
						result = ProcessRequest(data, client);
						log_record(client, result);
						string serverAnswer = to_string(result);
						client->messageLen= createSendMessage(client, serverAnswer);

						client->sentBytes = 0;					//the number of bytes which is sent to client
						client->operation = SEND;				//set operation to send reply message
						client->dataBuf.len = client->messageLen - client->sentBytes;
						if (WSASend(client->socket, &(client->dataBuf), 1, &transferredBytes, flags, &(client->overlapped), NULL) == SOCKET_ERROR) {
							if (WSAGetLastError() != WSA_IO_PENDING) {
								printf("WSASend() failed with error %d\n", WSAGetLastError());
								closesocket(client->socket);
								closeEventInArray(events, index);
								freeClientInfo(clients, index);
								client = 0;
								nEvents--;
								continue;
							}
						}

					}
				}


			}
			else {
				client->sentBytes += transferredBytes;
				// Post another I/O operation
				// Since WSASend() is not guaranteed to send all of the bytes requested,
				// continue posting WSASend() calls until all data are sent.
				if (client->messageLen > client->sentBytes) {
					client->dataBuf.buf = client->buffer + client->sentBytes;
					client->dataBuf.len = client->messageLen - client->sentBytes;
					client->operation = SEND;
					if (WSASend(client->socket, &(client->dataBuf), 1, &transferredBytes, flags, &(client->overlapped), NULL) == SOCKET_ERROR) {
						if (WSAGetLastError() != WSA_IO_PENDING) {
							printf("WSASend() failed with error %d\n", WSAGetLastError());
							closesocket(client->socket);
							closeEventInArray(events, index);
							freeClientInfo(clients, index);
							client = 0;
							nEvents--;
							continue;
						}
					}

				}
				else {
					// No more bytes to send, post another WSARecv() request
					memset(&(client->overlapped), 0, sizeof(WSAOVERLAPPED));
					client->overlapped.hEvent = events[index];
					client->messageLen = 0;
					client->operation = RECEIVE;
					client->dataBuf.buf = client->buffer;
					memset(client->buffer, 0, sizeof(client->buffer));
					client->dataBuf.len = TRANSID_SIZE;


					client->recvTransIDLeft = TRANSID_SIZE;
					client->recvDataLenLeft = DATALEN_SIZE;
					strcpy_s(client->transID_block, DATA_BUFSIZE, "");
					strcpy_s(client->dataLen_block, DATA_BUFSIZE, "");
					strcpy_s(client->data_block, DATA_BUFSIZE, "");

					flags = 0;


					if (WSARecv(client->socket, &(client->dataBuf), 1, &transferredBytes, &flags, &(client->overlapped), NULL) == SOCKET_ERROR) {
						if (WSAGetLastError() != WSA_IO_PENDING) {
							printf("WSARecv() failed with error %d\n", WSAGetLastError());
							closesocket(client->socket);
							closeEventInArray(events, index);
							freeClientInfo(clients, index);
							client = 0;
							nEvents--;
							continue;
						}
					}
				}

			}

		}
	}

}
