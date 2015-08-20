// Freedom.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <regex>

#pragma comment(lib, "Ws2_32.lib")

std::vector<std::string> feed_phost(std::string s_phost) {
	std::string phost = "";
	int breakpoint = rand() % (s_phost.length() - 1);
	return{ "Host: " + s_phost.substr(0, breakpoint) + s_phost.substr(breakpoint, s_phost.length() - breakpoint) + "\r\n" };
}
std::string generate_dummyheaders() {
	std::string dummy_header = "";
	for (int l = 0; l < 32; l++) {
		std::string dummy_key = "X-", dummy_value = "";
		static const char alpha_upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

		for (int i = 0; i < 16; i++) {
			dummy_key += alpha_upper[rand() % (sizeof(alpha_upper) - 1)];
		}
		for (int i = 0; i < 128; i++) {
			dummy_value += alphanum[rand() % (sizeof(alphanum) - 1)];
		}
		dummy_value += "\r\n";
		dummy_header += dummy_key + ": " + dummy_value;
	}
	return dummy_header;
}
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

std::string trim(std::string const& str)
{
	std::size_t first = str.find_first_not_of(' ');

	// If there is no non-whitespace character, both first and last will be std::string::npos (-1)
	// There is no point in checking both, since if either doesn't work, the
	// other won't work, either.
	if (first == std::string::npos)
		return "";

	std::size_t last = str.find_last_not_of(' ');

	return str.substr(first, last - first + 1);
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

bool handle_https(const std::vector<std::string> v_header) {
	return true;
}

std::vector<std::string> generate_new_header(const std::vector<std::string> v_header) {
	std::vector<std::string> v_head = tokenize(v_header[0]);
	std::string s_phost;
	std::vector<std::string> v_sreq;
	std::cout << v_header[0] << std::endl;;
	for (int i = 1; i < v_header.size(); i++) {
		std::vector<std::string> v_line = tokenize(v_header[i], ": ");

		if (std::regex_search(v_line[0], std::regex("[Hh][Oo][Ss][Tt]"))) {
			s_phost = trim(v_header[i].substr(5));
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
	if (s_phost.length() == 0) {
		s_phost = "127.0.0.1";
	}
	std::string s_path = v_head[1].substr(s_phost.length() + 7);
	std::string s_new_head = v_head[0] + " " + s_path + " " + v_head[2];
	std::vector<std::string> v_new_header;

	v_new_header.push_back(s_new_head + "\r\n");
	v_new_header.push_back(generate_dummyheaders());
	std::vector<std::string> phost = feed_phost(s_phost);
	v_new_header.insert(v_new_header.end(), phost.begin(), phost.end());
	for (int i = 0; i < v_sreq.size(); i++) {
		v_new_header.push_back(v_sreq[i] + "\r\n");
	}
	v_new_header.push_back("\r\n");
	return v_new_header;
}

std::vector<std::string> process_new_host(const std::vector<std::string> v_header) {
	std::string s_phost;
	std::string s_new_host, s_new_port;

	for (int i = 1; i < v_header.size(); i++) {
		if (std::regex_search(v_header[i], std::regex("[Hh][Oo][Ss][Tt]"))) {
			std::vector<std::string> v_line = tokenize(v_header[i], ": ");
			s_phost = v_header[i];
			if (std::regex_search(s_phost, std::regex("(.+?):([0-9]{1,5})"))) {
				s_new_host = tokenize(s_phost, ":")[1];
				s_new_port = tokenize(s_phost, ":")[2];
			}
			else {
				s_new_host = tokenize(s_phost, ":")[1];
				s_new_port = "80";
			}
		}
	}
	return{ trim(s_new_host),trim(s_new_port) };
}

int throw_response(const SOCKET ProxySocket, const SOCKET ClientSocket) {
	int iResult;
	char recvbuf[1024];
	int recvbuflen = sizeof(recvbuf);
	int content_length = 0;
	iResult = recv(ProxySocket, recvbuf, recvbuflen, 0);
	std::smatch m;
	std::string s_recvdata = std::string(recvbuf, iResult);
	std::regex_search(s_recvdata, m, std::regex("\r\n\r\n"));
	std::string s_header = s_recvdata.substr(0, m.position(0) + 4);
	std::vector<std::string> v_header = tokenize(s_header, "\r\n");
	std::string s_data = s_recvdata.substr(m.position(0) + 4);
	int recv_len = s_data.length();
	for (int i = 0; i < v_header.size(); i++) {
		if (std::regex_search(v_header[i], std::regex("[Cc][Oo][Nn][Tt][Ee][Nn][Tt]-[Ll][Ee][Nn][Gg][Tt][Hh]"))) {
			content_length = atoi(tokenize(v_header[i], ": ")[1].c_str());
		}
	}
	send(ClientSocket, s_header.c_str(), s_header.length(), 0);
	send(ClientSocket, s_data.c_str(), s_data.length(), 0);
	ZeroMemory(recvbuf, recvbuflen);
	while (recv_len < content_length || !std::regex_search(recvbuf, std::regex("\r\n$"))) {
		ZeroMemory(recvbuf, recvbuflen);
		iResult = recv(ProxySocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			recv_len += iResult;
		}
		else if (iResult == 0) {
			return 1;
		}
		else {
			continue;
		}
		iResult = send(ClientSocket, recvbuf, iResult, 0);
	}
	return 0;
}

bool validate_header(std::string header) {
	if (header.size() == 0) {
		return false;
	}
	std::vector<std::string> v_header = tokenize(header, "\r\n");
	if (v_header.size() < 4) {
		return false;
	}
}

int main(int argc, char* argv[])
{

	srand(time(0));
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

	freeaddrinfo(result);

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cout << "Listen failed with error: " << WSAGetLastError() << std::endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	SOCKET ClientSocket = INVALID_SOCKET;
	// Accept a client socket
	while (1) {
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
				header.append(recvbuf, iResult);
			}
			else if (iResult == 0) {
				printf("Connection closing...\n");
				closesocket(ClientSocket);
			}
			else {
				printf("recv failed: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				break;
				//WSACleanup();
				//return 1;
			}

		} while (!std::regex_search(header, std::regex("\r\n\r\n")));
		ZeroMemory(recvbuf, sizeof(recvbuf));
		std::vector<std::string> v_new_header;
		std::vector<std::string> s_new_host;
		if (!validate_header(header)) {
			continue;
		}
		std::vector<std::string> v_header = tokenize(header, "\r\n");
		std::string content;
		for (int i = 0; i < v_header.size(); i++) {
			if (std::regex_search(v_header[i], std::regex("Content-Length"))) {
				int content_length = atoi(trim(tokenize(v_header[i], ":")[1]).c_str());
				if (iResult < sizeof(recvbuf)) {
					std::smatch m;
					std::regex_search(header, m, std::regex("\r\n\r\n"));
					content = header.substr(m.position() + 4)+"\r\n";
					header = header.substr(0, m.position() + 4);
				}
			}
		}
		v_header = tokenize(header, "\r\n");
		if (std::regex_search(v_header[0], std::regex("[Cc][Oo][Nn][Nn][Ee][Cc][Tt]"))) {
			handle_https(v_header);
			closesocket(ClientSocket);
			continue;
			/*v_new_header = v_header;
			s_new_host = process_new_host(v_header);
			send(ClientSocket, "HTTP/1.1 200 Connection established\r\n\r\n", 40, 0);*/
		}
		else {
			v_new_header = generate_new_header(v_header);
			s_new_host = process_new_host(v_header);
		}

		//gethostbyname(s_new_host.c_str());
		iResult = getaddrinfo(s_new_host[0].c_str(), s_new_host[1].c_str(), &hints, &result);
		if (iResult != 0) {
			std::cout << "getaddrinfo failed: " << iResult << std::endl;
			WSACleanup();
			return 1;
		}
		SOCKET ProxySocket = INVALID_SOCKET;
		ProxySocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (ProxySocket == INVALID_SOCKET) {
			std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}
		iResult = connect(ProxySocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ProxySocket);
			ProxySocket = INVALID_SOCKET;
		}
		if (ProxySocket == INVALID_SOCKET) {
			std::cout << "Unable to connect to server!\n" << std::endl;
			WSACleanup();
			return 1;
		}
		for (int i = 0; i < v_new_header.size(); i++) {
			iResult = send(ProxySocket, v_new_header[i].c_str(), v_new_header[i].length(), 0);
		}
		iResult = send(ProxySocket, content.c_str(), content.length(), 0);
		if (iResult == SOCKET_ERROR) {
			printf("send failed: %d\n", WSAGetLastError());
			closesocket(ProxySocket);
			WSACleanup();
			return 1;
		}

		iResult = throw_response(ProxySocket, ClientSocket);
		closesocket(ProxySocket);
	}
	return 0;
}
