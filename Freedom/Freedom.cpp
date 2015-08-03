// Freedom.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <regex>

#pragma comment(lib, "Ws2_32.lib")


std::vector<std::string> tokenize(const std::string& str,
	const std::string& delimiters = " ")
{
	std::vector<std::string> tokens;
	// 맨 첫 글자가 구분자인 경우 무시
	std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// 구분자가 아닌 첫 글자를 찾는다
	std::string::size_type pos = str.find_first_of(delimiters, lastPos);

	while (std::string::npos != pos || std::string::npos != lastPos)
	{
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		lastPos = str.find_first_not_of(delimiters, pos);
		pos = str.find_first_of(delimiters, lastPos);
	}
	return tokens;
}

int validateArgs(int argc, char* argv[]) {
	if (argc > 3) {
		return -1;
	}
	else if (argc < 3) {
		argv[1] = "127.0.0.1";
		argv[2] = "8800";
	}
	return 0;
}

int main(int argc, char* argv[])
{

	validateArgs(argc, argv);

	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		std::cout << "WSAStartup failed: " << iResult << std::endl;
		return 1;
	}

	struct addrinfo *result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, argv[2], &hints, &result);
	if (iResult != 0) {
		std::cout << "getaddrinfo failed: " << iResult << std::endl;
		WSACleanup();
		return 1;
	}
	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		std::cout << "bind failed with error: " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cout << "Listen failed with error: " << WSAGetLastError() << std::endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	SOCKET ClientSocket = INVALID_SOCKET;
	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		std::cout << "accept failed: " << WSAGetLastError() << std::endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	char recvbuf[1024];
	int iSendResult;
	int recvbuflen = 1024;

	std::string header = std::string();
	// Receive until the peer shuts down the connection
	do {
		ZeroMemory(recvbuf, sizeof(recvbuf));
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			std::cout << "Bytes received: %d\n" << iResult << std::endl;

			header.append(recvbuf, iResult);
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}

	} while (!std::regex_search(header, std::regex("\r\n\r\n")));
	ZeroMemory(recvbuf, sizeof(recvbuf));

	std::vector<std::string> v_header = tokenize(header, "\r\n");
	std::vector<std::string> v_head = tokenize(v_header[0]);
	std::string s_phost;
	std::vector<std::string> v_sreq;
	for (int i = 1; i < v_header.size(); i++) {
		std::vector<std::string> v_line = tokenize(v_header[i], ": ");

		if (std::regex_search(v_line[0], std::regex("[Hh][Oo][Ss][Tt]"))) {
			s_phost = v_line[1];
		}
		else if (std::regex_search(v_line[0], std::regex("[Cc][Oo][Nn][Nn][Ee][Cc][Tt][Ii][Oo][Nn]"))) {
			if (std::regex_search(v_line[1], std::regex("(keep-alive|persist)"))) {
				v_sreq.push_back("Connection: close");
			}
			else {
				v_sreq.push_back(v_header[i]);
			}
		}
		else if (!std::regex_search(v_line[0], std::regex("(proxy-connection|PROXY-CONNECTION)"))) {
			v_sreq.push_back(v_header[i]);
		}
	}
	if (!std::regex_search(header, std::regex("[Cc][Oo][Nn][Nn][Ee][Cc][Tt][Ii][Oo][Nn]"))) {
		v_sreq.push_back("Connection: close");
	}
	if (s_phost.length() == 0) {
		s_phost = "127.0.0.1";
	}
	std::string s_path = v_head[1].substr(s_phost.length()+7);
	std::string s_new_head = v_head[0] + " " + s_path + " " + v_head[2];
	std::string s_new_host, s_new_port;
	if (std::regex_search(s_phost, std::regex("(.+?):([0-9]{1,5})"))) {
		s_new_host = tokenize(s_phost, ":")[0];
		s_new_port = tokenize(s_phost, ":")[1];
	}
	else {
		s_new_host = s_phost;
		s_new_port = "80";
	}

    return 0;
}

