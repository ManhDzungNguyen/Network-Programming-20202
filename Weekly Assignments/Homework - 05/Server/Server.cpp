#include "stdafx.h"
#include "Server.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "winsock2.h"
#include "ws2tcpip.h"
#include "windows.h"
#include "stdio.h"
#include "conio.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <ctime>
#include <fstream>
using namespace std;

#define WM_SOCKET WM_USER + 1
#define SERVER_PORT 6000
#define MAX_CLIENT 1024
#define BUFF_SIZE 2048
#define SERVER_ADDR "127.0.0.1"
#define TRANSID_SIZE 10 
#define DATALEN_SIZE 10 
#define ACCOUNT_FILE "account.txt"
#define LOG_FILE "log_20183899.txt"
#pragma comment(lib, "Ws2_32.lib")


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
HWND				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	windowProc(HWND, UINT, WPARAM, LPARAM);


typedef struct client_session_management {
	bool free;
	SOCKET socket;
	int clientPort;
	char* clientIP;
	string username;
	bool isLogin;
} c_ses;

c_ses client[MAX_CLIENT];
SOCKET listenSock;



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


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	MSG msg;
	HWND serverWindow;

	//Registering the Window Class
	MyRegisterClass(hInstance);

	//Create the window
	if ((serverWindow = InitInstance(hInstance, nCmdShow)) == NULL)
		return FALSE;

	//Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		MessageBox(serverWindow, L"Winsock 2.2 is not supported.", L"Error!", MB_OK);
		return 0;
	}

	//Construct socket	
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//requests Windows message-based notification of network events for listenSock
	WSAAsyncSelect(listenSock, serverWindow, WM_SOCKET, FD_ACCEPT | FD_CLOSE | FD_READ);

	//Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr)))
	{
		MessageBox(serverWindow, L"Cannot associate a local address with server socket.", L"Error!", MB_OK);
	}

	//Listen request from client
	if (listen(listenSock, MAX_CLIENT)) {
		MessageBox(serverWindow, L"Cannot place server socket in state LISTEN.", L"Error!", MB_OK);
		return 0;
	}

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = windowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SERVER));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"WindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}


//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	int i;
	for (i = 0; i <MAX_CLIENT; i++)
		client[i] = { true, NULL, 0, "", "", false };

	hWnd = CreateWindow(L"WindowClass", L"Post Message Server", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
		return FALSE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return hWnd;
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_SOCKET	- process the events on the sockets
//  WM_DESTROY	- post a quit message and return
//
//

LRESULT CALLBACK windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	SOCKET connSock;
	sockaddr_in clientAddr;
	int ret, clientAddrLen = sizeof(clientAddr), i, clientPort;
	char *clientIP = new char[INET_ADDRSTRLEN];

	switch (message) {
	case WM_SOCKET:
	{
		if (WSAGETSELECTERROR(lParam)) {
			for (i = 0; i < MAX_CLIENT; i++)
				if ((!client[i].free) && (client[i].socket == (SOCKET)wParam)) {
					closesocket(client[i].socket);
					client[i] = { true, NULL, 0, "", "", false };
					continue;
				}
		}

		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_ACCEPT:
		{
			connSock = accept((SOCKET)wParam, (sockaddr *)&clientAddr, &clientAddrLen);
			if (connSock == INVALID_SOCKET) {
				break;
			}

			clientIP = inet_ntoa(clientAddr.sin_addr);
			clientPort = ntohs(clientAddr.sin_port);

			for (i = 0; i < MAX_CLIENT; i++)
				if (client[i].free) {
					client[i] = { false, connSock, clientPort, clientIP, "", false };

					//requests Windows message-based notification of network events for listenSock
					WSAAsyncSelect(client[i].socket, hWnd, WM_SOCKET, FD_READ | FD_CLOSE);
					break;
				}
			if (i == MAX_CLIENT)
				MessageBox(hWnd, L"Too many clients!", L"Notice", MB_OK);
		}
		break;

		case FD_READ:
		{
			for (i = 0; i < MAX_CLIENT; i++)
				if ((!client[i].free) && client[i].socket == (SOCKET)wParam)
					break;
			
			string buff;
			int requestID;

			ret = recv_finetune_server(client[i].socket, requestID, buff);
			if (ret > 0) {
				int result;
				result = ProcessRequest(buff, &client[i]); // Processing request

				// Send request processing result code
				string serverAnswer = to_string(result);
				ret = send_finetune_server(client[i].socket, requestID, serverAnswer);
				log_record(client[i], buff, result); // Record detail to log record file
			}
		}
		break;

		case FD_CLOSE:
		{
			for (i = 0; i < MAX_CLIENT; i++)
				if ((!client[i].free) && client[i].socket == (SOCKET)wParam) {
					closesocket(client[i].socket);
					client[i] = { true, NULL, 0, "", "", false };
					break;
				}
		}
		break;
		}
	}
	break;

	case WM_DESTROY:
	{
		PostQuitMessage(0);
		shutdown(listenSock, SD_BOTH);
		closesocket(listenSock);
		WSACleanup();
		return 0;
	}
	break;

	case WM_CLOSE:
	{
		DestroyWindow(hWnd);
		shutdown(listenSock, SD_BOTH);
		closesocket(listenSock);
		WSACleanup();
		return 0;
	}
	break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}
