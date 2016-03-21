#include "stdafx.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <string>
#include <regex>
#include <Windows.h>
#pragma comment(lib, "Ws2_32.lib")
#define BUFSIZE 4096
DWORD g_BytesTransferred = 0;
VOID CALLBACK FileIOCompletionRoutine(
	__in  DWORD dwErrorCode,
	__in  DWORD dwNumberOfBytesTransfered,
	__in  LPOVERLAPPED lpOverlapped
	);

VOID CALLBACK FileIOCompletionRoutine(
	__in  DWORD dwErrorCode,
	__in  DWORD dwNumberOfBytesTransfered,
	__in  LPOVERLAPPED lpOverlapped)
{
	_tprintf(TEXT("Error code:\t%x\n"), dwErrorCode);
	_tprintf(TEXT("Number of bytes:\t%x\n"), dwNumberOfBytesTransfered);
	g_BytesTransferred = dwNumberOfBytesTransfered;
}
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
			try {
				if (std::regex_search(v_line[1], std::regex("(keep-alive|persist)"))) {
					v_sreq.push_back("Connection: close");
				}
				else {
					v_sreq.push_back(v_header[i]);
				}
			}
			catch (...) {
				v_sreq.push_back("Connection: close");
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
	char recvbuf[BUFSIZE];
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


typedef struct
{
	SOCKET hClntSock;
	SOCKADDR_IN clntAddr;
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

typedef struct
{
	OVERLAPPED overlapped;
	char buffer[BUFSIZE];
	WSABUF wsaBuf;
} PER_IO_DATA, *LPPER_IO_DATA;

unsigned int __stdcall CompletionThread(LPVOID pComPort);

void ErrorHandling(char *message);

void change_firefox_setting() {
	char buf[1024];
	HANDLE hFind;
	WIN32_FIND_DATA data;

	std::wstring inner_dir;
	GetEnvironmentVariableA("APPDATA", buf, sizeof(buf));
	std::string conf_dir = buf;
	conf_dir.append("\\Mozilla\\Firefox\\Profiles\\*.*");
	std::wstring w_conf_dir = std::wstring(conf_dir.begin(), conf_dir.end());
	HANDLE h = FindFirstFile((LPCWSTR)w_conf_dir.c_str(), &data);
	if (h != INVALID_HANDLE_VALUE)
	{
		do
		{
			inner_dir = data.cFileName;
			if (inner_dir.length() > 2) {
				break;
			}

		} while (FindNextFile(h, &data));
	}
	else {
		std::cout << "cannot find firefox config file" << std::endl;
		exit(0);
	}
	w_conf_dir = w_conf_dir.substr(0, w_conf_dir.length() - 3);
	w_conf_dir.append(inner_dir);
	w_conf_dir.append(L"\\prefs.js");

	LPOFSTRUCT lpReOpenBuff;

	HANDLE hFile = CreateFile(w_conf_dir.c_str(),               // file to open
		GENERIC_READ,          // open for reading
		FILE_SHARE_READ,       // share for reading
		NULL,                  // default security
		OPEN_EXISTING,         // existing file only
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, // normal file
		NULL);                 // no attr. template

	char   ReadBuffer[20000] = { 0 };
	OVERLAPPED ol = { 0 };
	DWORD  dwBytesRead = 0;

	ReadFileEx(hFile, ReadBuffer, 19999, &ol, FileIOCompletionRoutine);
	CloseHandle(hFile);

	std::string config = ReadBuffer;
	std::vector<std::string> v_configs= tokenize(config, "\r\n");
	std::vector<std::string> v_new_configs;
	bool proxy_setted= false, ip_setted= false, port_setted = false;
	for (int i = 0; i < v_configs.size(); i++) {
		if (std::regex_search(v_configs[i], std::regex("\"network.proxy.type\""))) {
			v_new_configs.push_back("user_pref(\"network.proxy.type\", 1);");
			proxy_setted = true;
		}
		else if (std::regex_search(v_configs[i], std::regex("\"network.proxy.http_port\""))) {
			v_new_configs.push_back("user_pref(\"network.proxy.http_port\", 8800);");
			ip_setted = true;
		}
		else if (std::regex_search(v_configs[i], std::regex("\"network.proxy.http\""))) {
			v_new_configs.push_back("user_pref(\"network.proxy.http\", \"127.0.0.1\");");
			port_setted = true;
		}
		else{
			v_new_configs.push_back(v_configs[i]);
		}
	}
	if (ip_setted == false) {
		v_new_configs.push_back("user_pref(\"network.proxy.http\", \"127.0.0.1\");");
	}
	if (port_setted == false) {
		v_new_configs.push_back("user_pref(\"network.proxy.http_port\", 8800);");
	}
	if (proxy_setted == false) {
		v_new_configs.push_back("user_pref(\"network.proxy.type\", 1);");
	}
	hFile = CreateFile(w_conf_dir.c_str(),                // name of the write
		GENERIC_WRITE,          // open for writing
		0,                      // do not share
		NULL,                   // default security
		CREATE_ALWAYS,             // create new file only
		FILE_ATTRIBUTE_NORMAL,  // normal file
		NULL);                  // no attr. template
	DWORD dwBytesWritten = 0;
	for (int i = 0; i < v_new_configs.size(); i++) {
		v_new_configs[i].append("\r\n");
		WriteFile(
			hFile,           // open file handle
			v_new_configs[i].c_str(),      // start of data to write
			v_new_configs[i].length(),  // number of bytes to write
			&dwBytesWritten, // number of bytes that were written
			NULL);            // no overlapped structure
	}
	CloseHandle(hFile);
}

int main(int argc, char** argv)
{
	change_firefox_setting();
	WSADATA wsaData;
	HANDLE hCompletionPort;

	SYSTEM_INFO SystemInfo;

	SOCKADDR_IN servAddr;

	LPPER_IO_DATA PerIoData;

	LPPER_HANDLE_DATA PerHandleData;

	SOCKET hServSock;
	int RecvBytes;
	int i, Flags;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 50);


	GetSystemInfo(&SystemInfo);

	for (i = 0; i<70; i++)
		_beginthreadex(NULL, 0, CompletionThread, (LPVOID)hCompletionPort, 0, NULL);

	hServSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(atoi("8800"));

	bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr));
	listen(hServSock, 5);

	while (TRUE)
	{
		SOCKET hClntSock;
		SOCKADDR_IN clntAddr;
		int addrLen = sizeof(clntAddr);

		hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &addrLen);

		PerHandleData = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
		PerHandleData->hClntSock = hClntSock;
		memcpy(&(PerHandleData->clntAddr), &clntAddr, addrLen);

		CreateIoCompletionPort((HANDLE)hClntSock, hCompletionPort, (DWORD)PerHandleData, 0);

		PerIoData = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
		memset(&(PerIoData->overlapped), 0, sizeof(OVERLAPPED));
		PerIoData->wsaBuf.len = BUFSIZE;
		PerIoData->wsaBuf.buf = PerIoData->buffer;

		Flags = 0;

		WSARecv(PerHandleData->hClntSock,
			&(PerIoData->wsaBuf),
			1,
			(LPDWORD)&RecvBytes,
			(LPDWORD)&Flags,
			&(PerIoData->overlapped),
			NULL
			);

	}
	return 0;
}

unsigned int __stdcall CompletionThread(LPVOID pComPort)

{
	HANDLE hCompletionPort = (HANDLE)pComPort;

	DWORD BytesTransferred;
	LPPER_HANDLE_DATA PerHandleData;
	LPPER_IO_DATA PerIoData;
	DWORD flags;

	while (1)
	{
		GetQueuedCompletionStatus(hCompletionPort,    // Completion Port
			&BytesTransferred,
			(LPDWORD)&PerHandleData,
			(LPOVERLAPPED*)&PerIoData,
			INFINITE
			);

		if (BytesTransferred == 0)
		{
			closesocket(PerHandleData->hClntSock);
			free(PerHandleData);
			free(PerIoData);

			continue;
		}

		PerIoData->wsaBuf.buf[BytesTransferred] = '\0';

		printf("Recv[%s]\n", PerIoData->wsaBuf.buf);
		std::string header = PerIoData->wsaBuf.buf;
		int iResult;




		std::vector<std::string> v_new_header;
		std::vector<std::string> s_new_host;
		if (!validate_header(header)) {
			continue;
		}
		std::vector<std::string> v_header = tokenize(header, "\r\n");
		std::string content;
		if (std::regex_search(header, std::regex("CONNECT"))) {
			closesocket(PerHandleData->hClntSock);
			continue;
		}
		for (int i = 0; i < v_header.size(); i++) {
			if (std::regex_search(v_header[i], std::regex("Content-Length"))) {
				int content_length = atoi(trim(tokenize(v_header[i], ":")[1]).c_str());
				std::smatch m;
				std::regex_search(header, m, std::regex("\r\n\r\n"));
				content = header.substr(m.position() + 4) + "\r\n";
				header = header.substr(0, m.position() + 4);

			}
		}
		v_header = tokenize(header, "\r\n");

		v_new_header = generate_new_header(v_header);
		s_new_host = process_new_host(v_header);

		struct addrinfo *result = NULL, *ptr = NULL, hints;
		ZeroMemory(&hints, sizeof(hints));
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

		iResult = throw_response(ProxySocket, PerHandleData->hClntSock);

		//PerIoData->wsaBuf.len = BytesTransferred;
		//WSASend(PerHandleData->hClntSock, &(PerIoData->wsaBuf), 1, NULL, 0, NULL, NULL);

		//// RECEIVE AGAIN
		//memset(&(PerIoData->overlapped), 0, sizeof(OVERLAPPED));
		//PerIoData->wsaBuf.len = BUFSIZE;
		//PerIoData->wsaBuf.buf = PerIoData->buffer;

		//flags = 0;
		//WSARecv(PerHandleData->hClntSock,
		//	&(PerIoData->wsaBuf),
		//	1,
		//	NULL,
		//	&flags,
		//	&(PerIoData->overlapped),
		//	NULL
		//	);
	}
	return 0;
}

void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}