
#pragma once
#include "pch.h"
#include "FileReaders.h"


struct ISyncReadStream
{
	virtual int Read(void* buf, int size) = 0;

	uint64_t ReadULEB();
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

	SocketSyncReadStream(int port);
	~SocketSyncReadStream();
	void Accept();
	int Read(void* buf, int size);
};

struct StructureParser
{
	char ReadNext(ISyncReadStream* s);

	std::string name;
	std::string preview;
	char typeCode;
	char typeSizeBytes;
	uint64_t fileOffset;
	uint64_t arraySize;
};

struct FileStructureViewer : ui::Buildable
{
	FileStructureViewer()
	{
		::system("cd FRET_Plugins && set RAW=1 && a > ../sockdump.txt");
	}
	void Build(ui::UIContainer* ctx) override;

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
	void Parse(ISyncReadStream* srs, StructureParser& sp, Node* parent);

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

struct FileStructureViewer2 : ui::Buildable
{
	void Build(ui::UIContainer* ctx) override
	{
		auto& trv = ctx->Make<ui::TreeView>();
		trv.GetStyle().SetHeight(style::Coord::Percent(100));
		trv.SetDataSource(ds);
		trv.CalculateColumnWidths();
	}

	FileStructureDataSource* ds = nullptr;
};
