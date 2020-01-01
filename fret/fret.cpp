
#include "pch.h"


struct ISyncReadStream
{
	virtual int Read(void* buf, int size) = 0;
};

struct FileSyncReadStream : ISyncReadStream
{
	FILE* fp;

	FileSyncReadStream(const char* name)
	{
		fp = fopen(name, "rb");
	}
	~FileSyncReadStream()
	{
		if (fp)
			fclose(fp);
	}
	int Read(void* buf, int size)
	{
		return fp ? fread(buf, 1, size, fp) : 0;
	}
};

struct SocketSyncReadStream : ISyncReadStream
{
	SOCKET s = -1;
	SOCKET rs = -1;
	bool done = false;

	SocketSyncReadStream(int port)
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
		if (bind(s, (sockaddr*)&my_addr, sizeof(my_addr)) == -1)
		{
			fprintf(stderr, "failed to bind to port %d\n", port);
			exit(EXIT_FAILURE);
		}

		listen(s, 1);
	}
	~SocketSyncReadStream()
	{
		if (rs != -1)
		{
			closesocket(rs);
			rs = -1;
		}
		if (s != -1)
		{
			closesocket(s);
			s = -1;
		}
		WSACleanup();
	}
	void Accept()
	{
		sockaddr_storage oaddr;
		int addr_size = sizeof(oaddr);
		rs = ::accept(s, (sockaddr*)&oaddr, &addr_size);
	}
	int Read(void* buf, int size)
	{
		if (done)
			return 0;
		int s = size;
		auto p = (char*)buf;
		while (s)
		{
			int ret = ::recv(rs, p, s, 0);
			if (ret == 0)
			{
				done = true;
				break;
			}
			if (ret == -1)
			{
				fprintf(stderr, "failed to recv\n");
				continue;
			}
			p += ret;
			s -= ret;
		}
		return size - s;
	}
};

struct StructureParser
{
	static uint64_t ReadULEB(ISyncReadStream* s)
	{
		uint64_t result = 0;
		int shift = 0;
		for (;;)
		{
			uint8_t b;
			if (!s->Read(&b, 1))
				return 0;
			result |= uint64_t(b & 0x7f) << shift;
			if (!(b & 0x80))
				break;
			shift += 7;
		}
		return result;
	}
	char ReadNext(ISyncReadStream* s)
	{
		char type;
		if (!s->Read(&type, 1))
			return 0;

		if (type == '{')
		{
			name.resize((size_t)ReadULEB(s));
			if (s->Read(&name[0], name.size()) != name.size())
				return 0; // failed to read the whole string
		}
		else if (type == 'F')
		{
			if (!s->Read(&typeCode, 1))
				return 0;
			if (!s->Read(&typeSizeBytes, 1))
				return 0;
			typeSizeBytes = (typeSizeBytes - '0') * 8;
			fileOffset = ReadULEB(s);
			arraySize = ReadULEB(s);

			name.resize((size_t)ReadULEB(s));
			if (s->Read(&name[0], name.size()) != name.size())
				return 0; // failed to read the whole string

			preview.resize((size_t)ReadULEB(s));
			if (s->Read(&preview[0], preview.size()) != preview.size())
				return 0; // failed to read the whole string
		}
		else if (type == 'I')
		{
			name.resize((size_t)ReadULEB(s));
			if (s->Read(&name[0], name.size()) != name.size())
				return 0; // failed to read the whole string

			preview.resize((size_t)ReadULEB(s));
			if (s->Read(&preview[0], preview.size()) != preview.size())
				return 0; // failed to read the whole string
		}
		// no data for }
		return type;
	}

	std::string name;
	std::string preview;
	char typeCode;
	char typeSizeBytes;
	uint64_t fileOffset;
	uint64_t arraySize;
};

struct FileStructureViewer : ui::Node
{
	FileStructureViewer()
	{
		::system("cd FRET_Plugins && set RAW=1 && a > ../sockdump.txt");
	}
	void Render(UIContainer* ctx) override
	{
		FileSyncReadStream fsrs("sockdump.txt");
		StructureParser sp;
		int curLevel = 0;
		int openLevel = 0;
		size_t idalloc = 0;
		bool hasOpenData = !openness.empty();
		for (;;)
		{
			char type = sp.ReadNext(&fsrs);
			if (type == 0)
				break;
			switch (type)
			{
			case '{':
			{
				size_t id = idalloc++;
				if (!hasOpenData)
					openness.push_back(false);
				if (curLevel == openLevel)
				{
					auto* item = ctx->Push<ui::CollapsibleTreeNode>();
					if (ctx->LastIsNew())
						item->open = openness[id];
					else
						openness[id] = item->open;

					if (item->open)
						openLevel++;
					ctx->Text(sp.name.c_str());
				}
				curLevel++;
				break;
			}
			case '}':
				if (curLevel - 1 == openLevel)
				{
					ctx->Pop();
				}
				else if (curLevel == openLevel)
				{
					ctx->Pop();
					openLevel--;
				}
				curLevel--;
				break;
			case 'F':
				if (curLevel == openLevel)
				{
					std::stringstream text;
					text << sp.name << " (" << sp.typeCode << int(sp.typeSizeBytes);
					if (sp.arraySize > 1)
					{
						text << "[" << sp.arraySize << "]";
					}
					text << ") = " << sp.preview;
					ctx->Text(text.str().c_str());
				}
				break;
			}
		}
	}
	std::vector<bool> openness;
};

struct FileStructureDataSource : ui::TreeDataSource
{
	struct Node
	{
		~Node()
		{
			for (auto* ch : children)
				delete ch;
		}

		std::string name, type, preview;
		std::vector<Node*> children;
	};

	FileStructureDataSource(const char* path)
	{
		ParseAll(path);
	}

	static DWORD __stdcall func(void* arg)
	{
		auto path = (const char*)arg;
		auto ext = strrchr(path, '.');
		char buf[256];
		sprintf(buf, "cd FRET_Plugins && %s.exe -f \"%s\" -s 12345", ext + 1, path);
		::system(buf);
		return 0;
	}
	void ParseAll(const char* path)
	{
		SocketSyncReadStream ssrs(12345);
		HANDLE h = CreateThread(NULL, 1024 * 64, func, (void*)path, 0, NULL);
		ssrs.Accept();
		StructureParser sp;
		Parse(&ssrs, sp, &root);
		WaitForSingleObject(h, INFINITE);
	}
	void Parse(ISyncReadStream* srs, StructureParser& sp, Node* parent)
	{
		for (;;)
		{
			char type = sp.ReadNext(srs);
			if (type == 0)
				break;
			switch (type)
			{
			case '{':
			{
				auto* n = new Node;
				n->name = sp.name;
				parent->children.push_back(n);
				Parse(srs, sp, n);
				break;
			}
			case '}':
				return;
			case 'F':
			{
				auto* n = new Node;
				n->name = sp.name;
				std::stringstream text;
				text << sp.typeCode << int(sp.typeSizeBytes);
				if (sp.arraySize > 1)
				{
					text << "[" << sp.arraySize << "]";
				}
				n->type = text.str();
				n->preview = sp.preview;
				parent->children.push_back(n);
				break;
			}
			case 'I':
			{
				auto* n = new Node;
				n->name = sp.name;
				n->preview = sp.preview;
				parent->children.push_back(n);
				break;
			}
			}
		}
	}

	size_t GetNumCols() override { return 3; }
	std::string GetColName(size_t col) override
	{
		static std::string names[3] =
		{
			"Name",
			"Type",
			"Preview",
		};
		return names[col];
	}
	size_t GetChildCount(uintptr_t id) override
	{
		if (id == ROOT) id = uintptr_t(&root);
		return reinterpret_cast<Node*>(id)->children.size();
	}
	uintptr_t GetChild(uintptr_t id, size_t which) override
	{
		if (id == ROOT) id = uintptr_t(&root);
		return uintptr_t(reinterpret_cast<Node*>(id)->children[which]);
	}
	std::string GetText(uintptr_t id, size_t col) override
	{
		switch (col)
		{
		case 0: return reinterpret_cast<Node*>(id)->name;
		case 1: return reinterpret_cast<Node*>(id)->type;
		case 2: return reinterpret_cast<Node*>(id)->preview;
		default: return "";
		}
	}

	Node root;
};

struct FileStructureViewer2 : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		auto* trv = ctx->Make<ui::TreeView>();
		trv->GetStyle().SetHeight(style::Coord::Percent(100));
		trv->SetDataSource(ds);
		trv->CalculateColumnWidths();
	}

	FileStructureDataSource* ds = nullptr;
};

struct REFile
{
	REFile(std::string n)
	{
		path = n;
		name = n;
		ds = new FileStructureDataSource(n.c_str());
	}
	~REFile()
	{
		delete ds;
	}

	std::string path;
	std::string name;
	FileStructureDataSource* ds;
};

struct MainWindow : ui::NativeMainWindow
{
	MainWindow()
	{
		//files.push_back(new REFile("loop.wav"));
		files.push_back(new REFile("arch.tar"));
	}
	void OnRender(UIContainer* ctx) override
	{
		auto s = ctx->Push<ui::TabGroup>()->GetStyle();
		s.SetLayout(style::Layout::EdgeSlice);
		s.SetHeight(style::Coord::Percent(100));
		{
			ctx->Push<ui::TabList>();
			{
				for (auto* f : files)
				{
					ctx->Push<ui::TabButton>();
					ctx->Text(f->name);
					ctx->MakeWithText<ui::Button>("X");
					ctx->Pop();
				}
			}
			ctx->Pop();

			for (auto* f : files)
			{
				auto* p = ctx->Push<ui::TabPanel>();
				auto s = p->GetStyle();
				s.SetLayout(style::Layout::EdgeSlice);
				s.SetHeight(style::Coord::Percent(100));
				ctx->Make<FileStructureViewer2>()->ds = f->ds;
				ctx->Pop();
			}
		}
		ctx->Pop();
	}

	std::vector<REFile*> files;
};

int uimain(int argc, char* argv[])
{
	ui::Application app(argc, argv);
	MainWindow mw;
	mw.SetVisible(true);
	return app.Run();
}
