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

#pragma comment(lib, "Ws2_32.lib")

CRITICAL_SECTION critical;
vector<string> logged_in_users; //a list containing logged in username

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

	if (ret > 0) {
		transID_buff[ret] = 0;
		transID = atoi(transID_buff);

		// reading packet dataLen_block
		char dataLen_buff[DATALEN_SIZE + 1];
		ret = recv(s, dataLen_buff, DATALEN_SIZE, MSG_WAITALL);
		if (ret > 0) {
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
			}
		}
	}

	return ret;
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
 * Function:	checkUserLogin
 * Description:	check an user is in logged_in_users list or not
 * Return		true	if user is in logged_in_users list
				false	if user is not in logged_in_users list
 */
bool checkUserLogin(string acc) {
	for (auto it = begin(logged_in_users); it != end(logged_in_users); ++it) {
		if (acc.compare(*it) == 0) {
			return true;
		}
	}
	return false;
}


/**
 * Function:	addUser
 * Description:	add an user to logged_in_users list
 * Return		true	if adding successfully
				false	if user is already exist
 */
bool addUser(string acc) {
	if (checkUserLogin(acc))
		return false;

	logged_in_users.push_back(acc);
	return true;


}


/**
 * Function:	removeUser
 * Description:	remove an user from logged_in_users list
 * Return		true	if removing successfully
				false	if user is not in the list
 */
bool removeUser(string acc) {
	if (acc.compare("") == 0)
		return true;

	for (auto it = begin(logged_in_users); it != logged_in_users.end(); ) {
		if (acc.compare(*it) == 0) {
			it = logged_in_users.erase(it);
			return true;
		}
		else {
			++it;
		}
	}
	return false;
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
				if (checkUserLogin(username)) {
					return 14;
				}
				if (!addUser(username)) {
					return 40;
				}
				client -> isLogin = true;
				client -> username = username;
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
	if (!removeUser(client->username)) {
		return 40;
	}
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
void log_record(c_ses* client, string message, int result) {
	int clientPort = client->clientPort;
	char* clientIP = client->clientIP;
	ostringstream stringStream;
	stringStream << clientIP << ":" << clientPort << " " << getTime() << " $ " << message << " $ " << result;

	string log_message = stringStream.str();

	ofstream logFile;
	logFile.open(LOG_FILE, std::ios::app);
	logFile << log_message << endl;
	logFile.close();
}


/* clientThread - Thread to receive the request from client and echo result*/
unsigned __stdcall clientThread(void *param) {
	string buff;
	int requestID; // transaction ID of client request
	c_ses* client = (c_ses*)param;
	SOCKET connectedSocket = client->socket;
	int clientPort = client->clientPort;
	char* clientIP = client->clientIP;

	while (true) {
		// Receive client request
		int ret = recv_finetune_server(connectedSocket, requestID , buff);
		if (ret == SOCKET_ERROR) {
			printf("Error %d: Cannot receive data.\n", WSAGetLastError());
			break;
		}
		else if (ret == 0) {
			printf("Client disconnects.\n");
			break;
		}
		else if (buff.size() > 0) {
			int result;
			string endRequest = buff.substr(buff.size() - 3, 3);
			if (endRequest.compare("\r\n.") != 0) {
				cout << "Receive from client [" << clientIP << ":" << clientPort << "]: " << buff;
				cout << " || Incorrect mesage format found!" << endl;
				result = 40;
			}
			else {
				buff = buff.substr(0, buff.size() - 3);
				cout << "Receive from client [" << clientIP << ":" << clientPort << "]: " << buff << endl;

				EnterCriticalSection(&critical);
				result = ProcessRequest(buff, client); // Processing request
				LeaveCriticalSection(&critical);
			}

			// Send request processing result code
			string serverAnswer = to_string(result); 
			serverAnswer.append("\r\n.");
			ret = send_finetune_server(connectedSocket, requestID, serverAnswer);
			if (ret == SOCKET_ERROR)
				printf("Error %d: Cannot send data.\n", WSAGetLastError());

			log_record(client, buff, result); // Record detail to log record file
		}
	}

	client->free = true;

	EnterCriticalSection(&critical);
	if (!removeUser(client->username)) {
		cout << "Error 401" << endl;
	}
	LeaveCriticalSection(&critical);

	client->username = "";

	closesocket(connectedSocket);
	return 0;
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

	// Step 5: Communicate with client
	SOCKET connSock;
	SOCKADDR_IN clientAddr;

	char clientIP[INET_ADDRSTRLEN];
	string clientRequest, serverAnswer;
	int clientAddrLen = sizeof(clientAddr), clientPort;


	c_ses client[MAX_CLIENT];
	for (int i = 0; i < MAX_CLIENT; i++) {
		client[i] = { true, NULL, 0, "", "", false};

	}
	HANDLE myhandle[NUM_THREAD];
	InitializeCriticalSection(&critical);
	while (1) {
		for (int i = 0; i < MAX_CLIENT; i++) {
			if (client[i].free == true) {
				// accept request
				connSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);
				if (connSock == SOCKET_ERROR) {
					printf("[Error %d] Cannot permit incoming connection", WSAGetLastError());
				}
				else {
					inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
					clientPort = ntohs(clientAddr.sin_port);
					client[i] = { false, connSock, clientPort, clientIP, "", false };
					printf("Accept incoming connection from %s:%d\n", clientIP, clientPort);
					myhandle[i] = (HANDLE)_beginthreadex(0, 0, clientThread, (void *)&client[i], 0, 0);
				}

				break;
			}
		}

	}
	WaitForMultipleObjects(NUM_THREAD, myhandle, TRUE, INFINITE);
	DeleteCriticalSection(&critical);
	closesocket(listenSock);

	WSACleanup();

	return 0;
}
