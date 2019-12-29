
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <io.h>
#include <fcntl.h>

struct RecvSocket
{
	SOCKET s;
	SOCKET rs;
	
	RecvSocket()
	{
		s = -1;
		rs = -1;
	}
	void init(int port)
	{
		WSADATA wsadata;
		if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
		{
			fprintf(stderr, "failed to initialize Windows Sockets\n");
			exit(EXIT_FAILURE);
		}
		
		s = socket(PF_INET, SOCK_STREAM, 0);
		if (s == -1)
		{
			fprintf(stderr, "failed to create socket\n");
			exit(EXIT_FAILURE);
		}

		sockaddr_in my_addr;
		my_addr.sin_family = AF_INET;
		my_addr.sin_port = htons(port);
		my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		memset(my_addr.sin_zero, 0, sizeof my_addr.sin_zero);
		if (bind(s, (sockaddr*) &my_addr, sizeof(my_addr)) == -1)
		{
			fprintf(stderr, "failed to bind to port %d\n", port);
			exit(EXIT_FAILURE);
		}
		
		listen(s, 1);
	}
	void free()
	{
		if (s != -1)
		{
			closesocket(s);
			s = -1;
		}
		WSACleanup();
	}
	void accept()
	{
		sockaddr_storage oaddr;
		int addr_size = sizeof(oaddr);
		rs = ::accept(s, (sockaddr*) &oaddr, &addr_size);
	}
	void read()
	{
		char buf[1024];
		for (;;)
		{
			int ret = ::recv(rs, buf, 1024, 0);
			if (ret == 0)
				break;
			if (ret == -1)
			{
				fprintf(stderr, "failed to recv\n");
				continue;
			}
			dump(buf, ret);
		}
	}
	void dump(const char* buf, int size)
	{
		if (!getenv("RAW"))
		{
			for (int i = 0; i < size; i++)
			{
				if (buf[i] >= 32 && buf[i] < 127)
					fputc(buf[i], stdout);
				else
					printf("\\x%02X", (unsigned char) buf[i]);
			}
		}
		else
		{
			fwrite(buf, 1, size, stdout);
		}
	}
};

static DWORD __stdcall func(void* arg)
{
	system("wav.exe -f loop.wav -s 12345");
}

int main()
{
	system("g++ --std=c++11 wav.cpp -lws2_32 -o wav.exe");
	RecvSocket rs;
	rs.init(12345);
	
	if (getenv("RAW"))
		//freopen(NULL, "wb", stdout);
		_setmode(_fileno(stdout), _O_BINARY);
	
	HANDLE h = CreateThread(NULL, 1024 * 64, func, NULL, 0, NULL);
	rs.accept();
	rs.read();
	WaitForSingleObject(h, INFINITE);
	
	rs.free();
}
