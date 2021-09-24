#include "stdafx.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

#define TRANSID_SIZE 10 
#define DATALEN_SIZE 10 

#pragma comment(lib, "Ws2_32.lib")


/**
 * Function:	send_finetune_client
 * Description: an improved version of send function, fine-tuned to work with client only.
 * Param	s		A descriptor identifying a connected socket
 * Param	transID		transaction ID of previous message
 * Param	data	the data to be transmitted
 * Return		the data to be transmitted
 */
int send_finetune_client(SOCKET s, int& transID, string data) {
	/**
	 * The sent packet consist of a transaction ID section, a Data length section and a data section: 
	 * [message] = [transID_block(10 bytes) || dataLen_block(10 bytes) || data]
	 */

	// Create transaction ID field
	string transID_block;
	transID++; // this line make sure no client message has the same transID with another one
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
		int ret = send(s, buff+idx, nLeft, 0);
		if (ret == SOCKET_ERROR)
			return ret;
		nLeft -= ret;
		idx += ret;
	}


	return size;

}


/**
 * Function:	recv_finetune_client
 * Description: an improved version of recv function, fine-tuned to work with client only.
 * Param	s		A descriptor identifying a connected socket
 * Param	transID		transaction ID of last sent message
 * Param	buff	the buffer which receive the incoming data.
 * Return		the data to be received
 */
int recv_finetune_client(SOCKET s, int transID, string &buff) {
	// reading packet header to get the data size
	char transID_buff[TRANSID_SIZE + 1];
	int transID_mes = 0;
	int dataLen = 0;
	int ret = 0;

	while (transID_mes!= transID) {
		buff = "";
		// reading packet transID_block 
		int ret = recv(s, transID_buff, TRANSID_SIZE, MSG_WAITALL);
		if (ret == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT)
				printf("Time-out!\n");
			else
				printf("[Error %d] Cannot receive data\n", WSAGetLastError());
			break;
		}

		if (ret > 0) {
			transID_buff[ret] = 0;
			transID_mes = atoi(transID_buff);

			// reading packet dataLen_block
			char dataLen_buff[DATALEN_SIZE + 1];
			ret = recv(s, dataLen_buff, DATALEN_SIZE, MSG_WAITALL);
			if (ret == SOCKET_ERROR) {
				if (WSAGetLastError() == WSAETIMEDOUT)
					printf("Time-out!\n");
				else
					printf("[Error %d] Cannot receive data\n", WSAGetLastError());
				break;
			}

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
	}

	return ret;
}



/**
 * Function:	show_menu
 * Description: show all application mode and allow user to choose one.
 * Return		The user chosen mode
 */
int show_menu() {
	string modeStr;
	cout << " *** MENU *** " << endl;
	cout << "1. Log in" << endl;
	cout << "2. Post message" << endl;
	cout << "3. Logout" << endl;
	cout << "4. Exit" << endl << endl;
	cout << "Enter the number of mode you want to work with. ";
	cout << "For instance, enter '1' if you want to log in." << endl;
	cout << "Selected mode: ";
	cin >> modeStr;
	if (modeStr.size() > 1) {
		return -1;
	}
	int mode = atoi(modeStr.c_str());
	return mode;
}


/**
 * Function:	show_result
 * Description: show request processing result.
 * Param	reply	the message containing result code from server
 */
void show_result(string reply) {
	string endReply = reply.substr(reply.size() - 3, 3);
	if (endReply.compare("\r\n.") != 0)
		cout << "An error occurred when handling the reply message from server." << endl;
	else
		reply = reply.substr(0, reply.size() - 3);

	int return_code = atoi(reply.c_str());

	if (return_code == 10)
		cout << "Login Successfully!" << endl;
	else if (return_code == 11)
		cout << "Login Failed! Your account didn't exist." << endl;
	else if (return_code == 12)
		cout << "Login Failed! Your account has been locked." << endl;
	else if (return_code == 13)
		cout << "Login Failed! Another account is already signed in." << endl;
	else if (return_code == 14)
		cout << "Login Failed! Your account has been logged in another device." << endl;
	else if (return_code == 20)
		cout << "Post Message Successfully!" << endl;
	else if (return_code == 21)
		cout << "Post Message Failed! You are not logged in. Please log in and try again." << endl;
	else if (return_code == 30)
		cout << "Logout Successfully!" << endl;
	else if (return_code == 31)
		cout << "Logout Failed! You haven't logged in yet." << endl;
	else if (return_code == 40)
		cout << "An error occurred when handling the request." << endl;
}


/**
 * Function:	login
 * Description: login function, allow user to login by username
 * Param	s	A descriptor identifying a connected socket
 * Param	transID		transaction ID of last sent message
 */
void login(SOCKET s, int& transID) {
	string username;
	cout << "Username: ";
	cin.ignore();
	getline(std::cin, username);
	
	string reply;
	string data = "USER";
	data.append(" ");
	data.append(username);
	data.append("\r\n.");

	int ret = send_finetune_client(s, transID, data);

	if (ret == SOCKET_ERROR)
		printf("Error %d: Cannot send data\n", WSAGetLastError());

	ret = recv_finetune_client(s, transID, reply);

	if (reply.size() > 0)
		show_result(reply);

}


/**
 * Function:	post_message
 * Description: Post mesage function, allow user to post message after login successfully.
 * Param	s	A descriptor identifying a connected socket
 * Param	transID		transaction ID of last sent message
 */
void post_message(SOCKET s, int& transID) {
	string message;
	cout << "Message: ";
	cin.ignore();
	getline(std::cin, message);

	string reply;
	string data = "POST";
	data.append(" ");
	data.append(message);
	data.append("\r\n.");

	int ret = send_finetune_client(s, transID, data);

	if (ret == SOCKET_ERROR)
		printf("Error %d: Cannot send data\n", WSAGetLastError());

	ret = recv_finetune_client(s, transID, reply);
	if (reply.size() > 0)
		show_result(reply);
}


/**
 * Function:	logout
 * Description: Logout function, allow user to sign out of an account.
 * Param	s	A descriptor identifying a connected socket
 * Param	transID		transaction ID of last sent message
 */
void logout(SOCKET s, int& transID) {
	string reply;
	string data = "EXIT";
	data.append("\r\n.");

	int ret = send_finetune_client(s, transID, data);

	if (ret == SOCKET_ERROR)
		printf("Error %d: Cannot send data\n", WSAGetLastError());

	ret = recv_finetune_client(s, transID, reply);

	if (reply.size() > 0)
		show_result(reply);
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

	// Set time-out for receiving
	int tv = 3000; // Time-out interval: 10000ms
	setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv), sizeof(int));

	// Step 3: Specify server address
	const char* SERVER_ADDR = argv[1];
	const int SERVER_PORT = atoi(argv[2]);
	//const int SERVER_PORT = 5500;
	//const char* SERVER_ADDR = "127.0.0.1";

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
	int transID = 0;
	bool exit = false;
	int mode;

	while (!exit) {
		mode = show_menu();

		switch (mode) {
			case 1: // login
			{
				login(client, transID);
				break;
			}

			case 2: // post mesage
			{
				post_message(client, transID);
				break;
			}

			case 3: // log out
			{
				logout(client, transID);
				break;
			}

			case 4: // turn off the client
			{
				exit = true;
				break;
			}
			default:
			{
				cout << "Invalid Mode!" << endl;
				break;
			}
		}

		cout << "-----------------------" << endl << endl;
		
	}

	// Step 6: Close socket
	closesocket(client);

	// Step 7: Terminate Winsock
	WSACleanup();

	return 0;
}
