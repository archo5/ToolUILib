
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

struct FileStructureViewer : UINode
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

struct MainWindow : ui::NativeMainWindow
{
	void OnRender(UIContainer* ctx) override
	{
		ctx->Make<FileStructureViewer>();
	}
};

int uimain(int argc, char* argv[])
{
	ui::Application app(argc, argv);
	MainWindow mw;
	mw.SetVisible(true);
	return app.Run();
}
