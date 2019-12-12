
#define __STDC_FORMAT_MACROS 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <winsock2.h>

char* get_file_contents(const char* filename, int& size)
{
	FILE *fp = fopen(filename, "rb");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		char* contents = (char*) malloc(size);
		rewind(fp);
		fread(contents, 1, size, fp);
		fclose(fp);
		return contents;
	}
	throw errno;
}

#define READA(name, type, size) \
	type name[size]; \
	rnd_(#name, name, size);
#define READ(name, type) \
	type name; \
	rnd_(#name, &name, 1);
#define MARKA(name, type, size) \
	mark_<type>(#name, size);

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef int64_t s64;
typedef uint64_t u64;

struct SendSocket
{
	SOCKET s;
	
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
		if (connect(s, (sockaddr*) &my_addr, sizeof(my_addr)) == -1)
		{
			fprintf(stderr, "failed to connect to port %d\n", port);
			exit(EXIT_FAILURE);
		}
	}
	void free()
	{
		closesocket(s);
		s = -1;
		WSACleanup();
	}
	bool on() const
	{
		return s != -1;
	}
	
	void send_char(char code)
	{
		send_buf(&code, 1);
	}
	void send_uleb(u64 v)
	{
		u8 tmp[10];
		int size = 0;
		do
		{
			u8 byte = v & 0x7f;
			v >>= 7;
			if (v != 0)
				byte |= 0x80;
			tmp[size++] = byte;
		}
		while (v != 0);
		send_buf(tmp, size);
	}
	void send_u64(u64 v)
	{
		send_buf(&v, sizeof(v));
	}
	void send_str(const char* s)
	{
		send_str(s, strlen(s));
	}
	void send_str(const char* s, size_t len)
	{
		send_uleb(len);
		send_buf(s, len);
	}
	void send_buf(const void* data, size_t size)
	{
		auto* p = static_cast<const char*>(data);
		::send(s, p, size, 0);
	}
};

struct Reader
{
	char* data_;
	int size_ = 0;
	int at = 0;
	int level_ = 0;
	int maxpreviewchars_ = 64;
	char* previewbuf_;
	
	SendSocket socket_;
	
	void read(char* out, int count) { readb(out, sizeof(*out) * count); }
	void read(int16_t* out, int count) { readb(out, sizeof(*out) * count); }
	void read(uint16_t* out, int count) { readb(out, sizeof(*out) * count); }
	void read(int32_t* out, int count) { readb(out, sizeof(*out) * count); }
	void read(uint32_t* out, int count) { readb(out, sizeof(*out) * count); }
	void readb(void* out, int bytes)
	{
		if (at + bytes >= size_)
		{
			memset(out, 0, bytes);
			return;
		}
		memcpy(out, &data_[at], bytes);
		at += bytes;
	}
	template <class T> void netdump(T* v, int count)
	{
		int pchars = maxpreviewchars_;
		netdump_(v, count, pchars);
		if (pchars <= 0)
		{
			for (int i = 0; i < 3; i++)
				previewbuf_[pchars - 1 - i] = '.';
		}
		socket_.send_str(previewbuf_, maxpreviewchars_ - pchars);
	}
	void netdump_(char* v, int count, int& pchars)
	{
		for (int i = 0; i < count; i++)
			if (v[i] >= 32 && v[i] < 127)
				previewbuf_[maxpreviewchars_ - pchars--] = v[i];
			else
				pchars -= snprintf(previewbuf_ + maxpreviewchars_ - pchars, pchars, "\\x%02X", v[i]);
	}
	void netdump_(uint16_t* v, int count, int& pchars)
	{
		for (int i = 0; i < count && pchars > 0; i++)
			pchars -= snprintf(previewbuf_ + maxpreviewchars_ - pchars, pchars, "%" PRIu16 " ", v[i]);
	}
	void netdump_(uint32_t* v, int count, int& pchars)
	{
		for (int i = 0; i < count && pchars > 0; i++)
			pchars -= snprintf(previewbuf_ + maxpreviewchars_ - pchars, pchars, "%" PRIu32 " ", v[i]);
	}
	void dump(uint16_t* v, int count) { for (int i = 0; i < count; i++) printf("%" PRIu16 " ", v[i]); }
	void dump(int32_t* v, int count) { for (int i = 0; i < count; i++) printf("%" PRId32 " ", v[i]); }
	void dump(uint32_t* v, int count) { for (int i = 0; i < count; i++) printf("%" PRIu32 " ", v[i]); }
	void dump(char* v, int count)
	{
		fputc('"', stdout);
		for (int i = 0; i < count; i++)
			if (v[i] >= 32 && v[i] < 127)
				fputc(v[i], stdout);
			else
				printf("\\x%02X", v[i]);
		fputc('"', stdout);
	}
	char typecode(char*) { return 'u'; }
	char typecode(s8*) { return 's'; }
	char typecode(u8*) { return 'u'; }
	char typecode(u16*) { return 'u'; }
	char typecode(u32*) { return 'u'; }
	template <class T> void rnd_(const char* name, T* out, size_t size)
	{
		read(out, size);
		if (socket_.on())
		{
			socket_.send_char('F');
			socket_.send_char(typecode(out));
			socket_.send_char('0' + sizeof(T));
			socket_.send_uleb(at - size * sizeof(T));
			socket_.send_uleb(size);
			socket_.send_str(name);
			netdump(out, size);
		}
		else
		{
			printlev_();
			printf("%s = ", name);
			dump(out, size);
			puts("");
		}
	}
	template <class T> void mark_(const char* name, size_t size)
	{
		if (socket_.on())
		{
			socket_.send_char('F');
			socket_.send_char(typecode(static_cast<T*>(nullptr)));
			socket_.send_char('0' + sizeof(T));
			socket_.send_uleb(at);
			socket_.send_uleb(size);
			socket_.send_str(name);
			netdump(reinterpret_cast<T*>(&data_[at]), size);
		}
		else
		{
			printlev_();
			printf("%s = ", name);
			dump(reinterpret_cast<T*>(&data_[at]), size);
			puts("");
		}
		at += sizeof(T) * size;
	}
	void printlev_()
	{
		for (int i = 0; i < level_; i++)
			printf("    ");
	}
	void PUSH(const char* name)
	{
		if (socket_.on())
		{
			socket_.send_char('{');
			socket_.send_str(name);
		}
		else
		{
			printlev_();
			puts(name);
			level_++;
		}
	}
	void POP()
	{
		if (socket_.on())
		{
			socket_.send_char('}');
		}
		else
		{
			level_--;
		}
	}
	
	const char* _getopt(int argc, char** argv, int& i, const char* shrt, const char* lng)
	{
		if (strcmp(argv[i], shrt) == 0 && i + 1 < argc)
		{
			i++;
			return argv[i];
		}
		size_t ll = strlen(lng);
		if (strncmp(argv[i], lng, ll) == 0 && argv[i][ll] == '=')
		{
			return argv[i] + ll + 1;
		}
		return nullptr;
	}
	int _main_(int argc, char** argv)
	{
		const char* filename = nullptr;
		const char* type = nullptr;
		int send_port = 0;
		for (int i = 1; i < argc; i++)
		{
			if (auto* a = _getopt(argc, argv, i, "-f", "--file")) { filename = a; continue; }
			if (auto* a = _getopt(argc, argv, i, "-p", "--position")) { at = atoi(a); continue; }
			if (auto* a = _getopt(argc, argv, i, "-t", "--type")) { type = a; continue; }
			if (auto* a = _getopt(argc, argv, i, "-s", "--sendport")) { send_port = atoi(a); continue; }
		}
		if (filename == nullptr)
		{
			fprintf(stderr, "error: no file specified\n");
			return EXIT_FAILURE;
		}
		data_ = get_file_contents(filename, size_);
		if (send_port)
		{
			socket_.init(send_port);
		}
		previewbuf_ = new char[maxpreviewchars_];
		parse(type);
		delete [] previewbuf_;
		socket_.free();
		return EXIT_SUCCESS;
	}
	
	// "at" is set to starting position before call
	virtual void parse(const char* type) = 0;
};

#define DEFINE_PLUGIN(cls) \
	int main(int argc, char** argv) { cls inst; return inst._main_(argc, argv); }
