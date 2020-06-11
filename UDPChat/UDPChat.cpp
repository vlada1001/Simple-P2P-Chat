// UDPChat.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <cstring>
#include <iostream>
#include <string>
#include <winsock2.h>
#include <Ws2def.h>
#include <ws2tcpip.h>

// ili ga ubaci rucno za ceo projekat
#pragma comment(lib, "ws2_32.lib")

constexpr auto IP_TARGET = "127.0.0.1";
constexpr int BUFFER_SIZE = 1024;
constexpr int TIMEOUT = 3000;
BOOL END = FALSE; // thread exit flag

SOCKET make_sock(const WORD port)
{
	SOCKET sock = (SOCKET)NULL;
	SOCKADDR_IN address {0};

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
	{
		return (SOCKET)NULL;
	}

	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	//address.sin_addr.s_addr = inet_addr(IP_TARGET);
	inet_pton(AF_INET, IP_TARGET, &(address.sin_addr));
	
	if (bind(sock, reinterpret_cast<SOCKADDR*>(&address),
	         sizeof(address)) == SOCKET_ERROR)
	{
		closesocket(sock);
		return (SOCKET)NULL;
	}

	return sock;
}

BOOL send_data(SOCKET sock, const WORD w_dst_port)
{
	SOCKADDR_IN send_address = {0};
	char buffer[BUFFER_SIZE];

	send_address.sin_family = AF_INET;
	send_address.sin_port = htons(w_dst_port);
	//send_address.sin_addr.s_addr = inet_addr(IP_TARGET);
	inet_pton(AF_INET, IP_TARGET, &(send_address.sin_addr));

	printf_s("Unesi poruku: ");
	fgets(buffer, BUFFER_SIZE, stdin);

	if (buffer[0] == 'q')
	{
		std::string s = "Korisnik je izasao\n";
		auto tmp = s.c_str();
		sendto(sock, tmp, strlen(tmp), 0, (SOCKADDR*)&send_address, sizeof(send_address));
		return FALSE;
	}

	sendto(sock, buffer, strlen(buffer), 0, (SOCKADDR*)&send_address, sizeof(send_address));

	return TRUE;
}

DWORD WINAPI receiver_thread(LPVOID param)
{
	SOCKET sock = (SOCKET)param;
	SOCKADDR_IN receiver_address = { 0 };
	int ret, receiver_size;
	char buffer[BUFFER_SIZE];

	while (!END)
	{
		receiver_size = sizeof(receiver_address);
		ret = recvfrom(sock, buffer, BUFFER_SIZE, 0, (SOCKADDR*)&receiver_address, &receiver_size);

		if (ret == SOCKET_ERROR)
			continue;

		buffer[ret] = '\0';
		//printf_s("\n[%s:%d] : %s", inet_ntoa(receiver_address.sin_addr), htons(receiver_address.sin_port), buffer);
		char tmp[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(receiver_address.sin_addr), tmp, INET_ADDRSTRLEN);
		printf_s("\n[%s:%d] : %s", tmp, htons(receiver_address.sin_port), buffer);
		printf_s("Unesi poruku: ");
	}

	printf_s("Kraj prenosa ...\n");

	return 0;
}

// argv[0]: "udpchat"
// argv[1]: source port
// argv[2]: destination port
int main(int argc, char** argv)
{
	WSADATA wsa_data = {0};
	WORD w_src_port;
	WORD w_dst_port;
	
	if (argc != 3)
	{
		printf_s("Morate uneti adrese u ovom formatu [source port] [destination port] ...");
		int ok = false;
		int src = 0;
		int dest = 0;

		while (src == dest)
		{
			scanf_s("%d", &src);
			scanf_s("%d", &dest);
		}

		w_src_port = static_cast<WORD>(src);
		w_dst_port = static_cast<WORD>(dest);

		
	}
	else
	{
		// ToDo: izbegni conversion error
		w_src_port = static_cast<WORD>(atoi(argv[1])); // moguc coversion error u atoi
		w_dst_port = static_cast<WORD>(atoi(argv[2]));
	}

	std::cout << w_src_port << " -> " << w_dst_port << "\n";
	
	WSAStartup(MAKEWORD(2, 2), &wsa_data);

	auto sock = make_sock(w_src_port);

	if (sock)
	{
		auto h_thread = CreateThread(NULL, 0, receiver_thread,
		                                   PVOID(sock), 0, NULL);

		while (true)
		{
			if (!send_data(sock, w_dst_port))
				break;
		}

		END = TRUE;
		closesocket(sock);

		for (;;) {
			switch (WaitForSingleObject(h_thread, TIMEOUT)) {
			case WAIT_TIMEOUT:
				printf_s("*** Greska: Kraj prenosa ...\n");
				break;
			default: return 0; // izadji iz thread-a
			}
		}
	}

	WSACleanup();

	return 0;
}
