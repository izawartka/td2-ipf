#define _CRT_SECURE_NO_WARNINGS	// ignore scanf warnings in visual studio
#include <stdio.h>      // for printf
#include <winsock2.h>   // the networking library.
#include <Ws2tcpip.h>	// more tcp/ip functionality
#pragma comment(lib, "ws2_32.lib") // links WinSock2 with MS Visual Studio
#include <vector>       // for the server's client list and input buffer
#include <conio.h>      // for kbhit() and getch()
#include "Serial.h"
#include <string>
#include "INIReader.h"
#include <stdlib.h>
#include <iostream>
#include "version.h"
using namespace std;

INIReader reader("config.ini");
string INIcom;
int INIport;
string INIip;
char SETcom[] = "\\\\.\\COM6";
const char* SETip = "192.168.1.69";
int clientLogic(SOCKET mySocket, const sockaddr* connectionAddr);
const int STATUS_READ = 0x1, STATUS_WRITE = 0x2, STATUS_EXCEPT = 0x4; // used by getStatus
int getStatus(const SOCKET a_socket, int status);
void finalWSACleanup();

void ipLoad(const char * buf) {
//    printf("Given Str = %s", buf);
    SETip=buf;
}
int main()
{
    INIcom = reader.Get("CONFIG", "com", "UNKNOWN");
    INIport = reader.GetInteger("CONFIG", "port", -1);
    INIip = reader.Get("CONFIG", "ip", "UNKNOWN");
//    cout << INIcom << endl;
//    cout << INIport << endl;
//    cout << INIip << endl;

    char* MEMcom;
    MEMcom = &INIcom[0];
    strcpy(SETcom, MEMcom);
 //   cout << SETcom << endl;

    char* MEMip;
    MEMip = &INIip[0];
    ipLoad(MEMip);
//   cout << SETip << endl;

	sockaddr_in connectionAddress;
	int connectionPort = INIport;

	// initialize the socket's address
	memset(&connectionAddress, 0, sizeof(sockaddr_in)); // initialize to zero
	connectionAddress.sin_family = AF_INET;
	// host-to-network-short: big-endian conversion of a 16 byte value
	connectionAddress.sin_port = htons(connectionPort);
	// initialize the server socket
	int result;
	WSADATA wsaData; // gets populated w/ info explaining this sockets implementation
	// load Winsock 2.0 DLL. initiates use of the Winsock DLL by a process
	if ((result = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
	{
		printf("WSAStartup() error %d\n", result);
		return EXIT_FAILURE;
	}
	atexit(finalWSACleanup); // add callback to trigger when program ends. cleans up sockets
	// create the main socket, either client or server
	SOCKET mySocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (mySocket == INVALID_SOCKET)
	{
		printf("socket() error %d\n", WSAGetLastError());
		return EXIT_FAILURE;
	}
	unsigned long argp = 1;
	result = setsockopt(mySocket,SOL_SOCKET,SO_REUSEADDR,(char*)&argp, sizeof(argp));
	if (result != 0)
	{
		printf("setsockopt() error %d\n", result);
		return EXIT_FAILURE;
	}
	argp = 1;
	if (ioctlsocket(mySocket,FIONBIO,&argp) == SOCKET_ERROR)
	{
		printf("ioctlsocket() error %d\n", WSAGetLastError());
		return EXIT_FAILURE;
	}

	// connect to the server
	const char* targetIP = SETip; // "::1"; // IPv6 localhost doesn't appear to work...
	unsigned long raw_ip_nbo;// = inet_addr(targetIP); // inet_addr is an old method for IPv4
	inet_pton(AF_INET, targetIP, &raw_ip_nbo); // IPv6 method of address acquisition
	if (raw_ip_nbo == INADDR_NONE)
	{
		printf("inet_addr() error \"%s\"\n", targetIP);
		return EXIT_FAILURE;
	}
	connectionAddress.sin_addr.s_addr = raw_ip_nbo;
	result = clientLogic(mySocket, (const sockaddr*)&connectionAddress);

	if (result == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}
	if (mySocket != INVALID_SOCKET)
	{
		result = closesocket(mySocket); // server closes, it doesn't shut down
		if (result != 0)
		{
			printf("closesocket() error %d\n", WSAGetLastError());
			return EXIT_FAILURE;
		}
		mySocket = INVALID_SOCKET;
	}
	return EXIT_SUCCESS;
}

int clientLogic(SOCKET mySocket, const sockaddr* connectionAddress)
{
	char SPincomingData[256] = "";
	int SPreadResult = 0;
	int SPdataLength = 256;
	Serial* SP = new Serial(SETcom);

	if (SP->IsConnected()) printf("Ustanowiono polaczenie szeregowe!\n");


	int result, errorCode, connectionAttempts = 0;
	bool connectionWaiting = false;
	// initial connection loop
	do
	{
		// allow user to quit gracefully with the escape key
		if (_kbhit() && _getch() == 27)
		{
			return EXIT_FAILURE;
		}
		if (!connectionWaiting)
		{
			result = connect(mySocket, connectionAddress, sizeof(sockaddr_in));
			errorCode = WSAGetLastError();
		}
		else
		{
			result = getStatus(mySocket, STATUS_WRITE);
			if (result != 0)
			{
				errorCode = result = WSAEISCONN;
			}
		}
		switch (errorCode)
		{
		case 0:
			connectionWaiting = true;
			break;
		case WSAEISCONN:
			printf("Ustanowiono polaczenie TCP/IP!                 \n");
			result = WSAEISCONN;
			break;
		case WSAEWOULDBLOCK:
		case WSAEALREADY:
			printf("oczekiwanie na polaczenie... (nacisnij esc aby anulowac)         \r");
			connectionWaiting = true;
			break;
		case WSAEINVAL:
			printf("\nniepoprawny argument\n");
			return EXIT_FAILURE;
		default:
			printf("\nblad polaczenia klienta %d\n", errorCode);
			return EXIT_FAILURE;
		}
	} while (result == SOCKET_ERROR || result == 0);
	// client loop
	int iterations = 0;
	bool sendit = false;
	char userTextField[1024];
	userTextField[0] = '\0';
	int userTextFieldCursor = 0;
	int userInput;
	while (getStatus(mySocket, STATUS_READ) != SOCKET_ERROR)
	{
		SPreadResult = SP->ReadData(SPincomingData, SPdataLength);
		SPincomingData[SPreadResult] = 0;
		if (SPreadResult > 0)
		{
			printf("=> %s", SPincomingData);
			SPincomingData[SPreadResult] = '\r';
			SPincomingData[SPreadResult + 1] = '\n';
			SPreadResult += 2;

			result = send(mySocket,
				(const char*)SPincomingData, SPreadResult, 0);
			if (result == SOCKET_ERROR)
			{
				printf("blad nadawania klienta %d\n", WSAGetLastError());
				return EXIT_FAILURE;
			}
		}


		/*// if user types, remember what's being typed, send with enter.
		if (_kbhit())
		{
			userInput = _getch();
			sendit = userInput == '\n' || userInput == '\r';
			if (!sendit)
			{
				putchar(userInput);
				userTextField[userTextFieldCursor++] = userInput;
				userTextField[userTextFieldCursor] = '\0';
				if (userTextFieldCursor >= sizeof(userTextField) - 1)
				{
					sendit = true;
				}
			}
			if (sendit)
			{
				userTextField[userTextFieldCursor] = '\r';
				userTextField[userTextFieldCursor+1] = '\n';
				userTextFieldCursor += 2;

				result = send(mySocket,
					(const char*)userTextField, userTextFieldCursor, 0);
				if (result == SOCKET_ERROR)
				{
					printf("client send() error %d\n", WSAGetLastError());
					return EXIT_FAILURE;
				}

				userTextFieldCursor = 0;
				userTextField[userTextFieldCursor] = '\0';
			}
		}*/
		// receive data from the server, if there is any
		if (getStatus(mySocket, STATUS_READ) == 1)
		{
			unsigned long howMuchInBuffer = 0;
			unsigned long numBytesRead = 0;
			char collectingBuffer[256];
			char* sendToArduinoBuffer;
			printf("<= ");
			do
			{
				ioctlsocket(mySocket, FIONREAD, &howMuchInBuffer);
				// 4 bytes at a time out of the socket, stored in userInput
				int result = recv(mySocket,	(char*)(&userInput), sizeof(userInput), 0);
				// NOTE: for multi-byte values sent across the network
				// ntohl or ntohs should be used to convert 4-byte and
				// 2-byte values respectively.
				if (result == SOCKET_ERROR)
				{
					printf("blad odbioru klienta %d\n", WSAGetLastError());
					return EXIT_FAILURE;
				}
				for (int i = 0; i < result; ++i)
				{
					collectingBuffer[numBytesRead] = ((char*)(&userInput))[i];
					numBytesRead++;
				}
				howMuchInBuffer -= result;
			} while (howMuchInBuffer > 0);

			sendToArduinoBuffer = new char[numBytesRead - 2];
			printf(sendToArduinoBuffer, numBytesRead - 2, collectingBuffer);
			sendToArduinoBuffer[numBytesRead - 3] = '\n';
			printf("%s", sendToArduinoBuffer, numBytesRead-2);
			SP->WriteData(sendToArduinoBuffer, numBytesRead-2);
			//SP->WriteData(sendToArduinoBuffer, sizeof(sendToArduinoBuffer));

			numBytesRead = 0;
		}
		else
		{
			iterations++;
			printf("%s                \r", userTextField);
		}
	}
	return EXIT_SUCCESS;
}

// status: 0x1 for read, 0x2 for write, 0x4 for exception
int getStatus(const SOCKET a_socket, int status)
{
	// zero seconds, zero milliseconds. max time select call allowd to block
	static timeval instantSpeedPlease = { 0,0 };
	fd_set a = { 1, {a_socket} };
	fd_set* read = ((status & 0x1) != 0) ? &a : NULL;
	fd_set* write = ((status & 0x2) != 0) ? &a : NULL;
	fd_set* except = ((status & 0x4) != 0) ? &a : NULL;
	/*
	select returns the number of ready socket handles in the fd_set structure, zero if the time limit expired, or SOCKET_ERROR if an error occurred. WSAGetLastError can be used to retrieve a specific error code.
	*/
	int result = select(0, read, write, except, &instantSpeedPlease);
	if (result == SOCKET_ERROR)
	{
		result = WSAGetLastError();
	}
	if (result < 0 || result > 3)
	{
		printf("select(read) error %d\n", result);
		return SOCKET_ERROR;
	}
	return result;
}

void finalWSACleanup() // callback used to clean up sockets
{
	int result = WSACleanup();
	if (result != 0)
	{
		printf("WSACleanup() error %d\n", result);
	}
}

/*
void main()
{
	///PRZYGOTOWANIE POŁĄCZENIA SERIAL///



	///PRZYGOTOWANIE POŁĄCZENIA TCP///

	string ipAddress = "192.168.0.101";			// IP Address of the server
	int port = 7424;						// Listening port # on the server

	// Initialize WinSock
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0)
	{
		cout << "Can't start Winsock, Err #" << wsResult << endl;
		return;
	}

	// Create socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		cout << "Can't create socket, Err #" << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	// Fill in a hint structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

	// Connect to server
	int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR)
	{
		cout << "Can't connect to server, Err #" << WSAGetLastError() << endl;
		closesocket(sock);
		WSACleanup();
		return;
	}
	else
	{
		cout << "TCP/IP Connection ready!" << endl;
	}

	///BUFORY DLA PRZESYŁANYCH DANYCH///

	char incomingData[256] = "";			// don't forget to pre-allocate memory
	//printf("%s\n",incomingData);
	int readResult = 0;
	int dataLength = 256;

	// Do-while loop to send and receive data
	char buf[4096] = "";

	while(true)
	{
		int bytesReceived = recv(sock, buf, 4096, 0);
		if (bytesReceived > 0)
		{
			// Echo response to console
			cout << "=> " << string(buf, 0, bytesReceived);
			buf[bytesReceived] = '\n';
			bytesReceived++;
			SP->WriteData(buf, bytesReceived);
		}

		readResult = SP->ReadData(incomingData, dataLength);
		incomingData[readResult] = 0;
		printf("%s", incomingData);

		cout << "iteracja!" << endl;
	}

	// Gracefully close down everything
	closesocket(sock);
	WSACleanup();
}*/
