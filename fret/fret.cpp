
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

Color4f colorFloat32{ 0.9f, 0.1f, 0.0f, 0.3f };
Color4f colorInt16{ 0.3f, 0.1f, 0.9f, 0.3f };
Color4f colorInt32{ 0.1f, 0.3f, 0.9f, 0.3f };
Color4f colorASCII{ 0.1f, 0.9f, 0.0f, 0.3f };

enum DataType
{
	DT_I16,
	DT_U16,
	DT_I32,
	DT_U32,
	DT_F32,
};
uint8_t typeSizes[] = { 2, 2, 4, 4, 4 };

struct Marker
{
	DataType type;
	uint64_t at;
	uint64_t count;
	uint64_t repeats;
	uint64_t stride;

	bool Contains(uint64_t pos) const
	{
		if (pos < at)
			return false;
		pos -= at;
		if (stride)
		{
			if (pos >= stride * repeats)
				return false;
			pos %= stride;
		}
		return pos < typeSizes[type] * count;
	}
	Color4f GetColor() const
	{
		switch (type)
		{
		case DT_I16: return colorInt16;
		case DT_U16: return colorInt16;
		case DT_I32: return colorInt32;
		case DT_U32: return colorInt32;
		case DT_F32: return colorFloat32;
		default: return { 0 };
		}
	}
};

ui::DataCategoryTag DCT_MarkedItems[1];
struct MarkerData
{
	MarkerData()
	{
		markers.push_back({ DT_I32, 8, 1, 1, 0 });
		markers.push_back({ DT_F32, 12, 3, 2, 12 });
	}

	Color4f GetMarkedColor(uint64_t pos)
	{
		for (const auto& m : markers)
		{
			if (m.Contains(pos))
				return m.GetColor();
		}
		return Color4f::Zero();
	}
	bool IsMarked(uint64_t pos, uint64_t len)
	{
		// TODO optimize
		for (uint64_t i = 0; i < len; i++)
		{
			for (const auto& m : markers)
				if (m.Contains(pos + i))
					return true;
		}
		return false;
	}
	void AddMarker(DataType dt, uint64_t from, uint64_t to)
	{
		markers.push_back({ dt, from, (to - from) / typeSizes[dt], 1, 0 });
		ui::Notify(DCT_MarkedItems, this);
	}

	std::vector<Marker> markers;
};

struct IDataSource
{
	virtual void Read(uint64_t at, size_t size, void* out) = 0;
};

struct FileDataSource : IDataSource
{
	void Read(uint64_t at, size_t size, void* out) override
	{
		_fseeki64(fp, at, SEEK_SET);
		size_t rd = fread(out, 1, size, fp);
		if (rd < size)
			memset((char*)out + rd, 0, size - rd);
	}

	FILE* fp;
};

struct BuiltinTypeInfo
{
	uint8_t size;
	bool separated;
	void(*append_to_str)(const void* src, std::string& out);
};

static std::unordered_map<std::string, BuiltinTypeInfo> g_builtinTypes
{
	{ "pad", { 1, false, [](const void* src, std::string& out) {} } },
	{ "char", { 1, false, [](const void* src, std::string& out) { out.push_back(*(const char*)src); } } },
	{ "i8", { 1, true, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRId8, *(const int8_t*)src); out += bfr; } } },
	{ "u8", { 1, true, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRIu8, *(const uint8_t*)src); out += bfr; } } },
	{ "i16", { 2, true, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRId16, *(const int16_t*)src); out += bfr; } } },
	{ "u16", { 2, true, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRIu16, *(const uint16_t*)src); out += bfr; } } },
	{ "i32", { 4, true, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRId32, *(const int32_t*)src); out += bfr; } } },
	{ "u32", { 4, true, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRIu32, *(const uint32_t*)src); out += bfr; } } },
};

ui::DataCategoryTag DCT_Struct[1];
struct DataDesc
{
	struct Field
	{
		std::string type;
		std::string name;
		uint64_t count = 1;
		uint64_t off = 0;
	};
	struct Struct
	{
		bool serialized = false;
		std::vector<Field> fields;
		uint64_t size = 0;
	};
	struct StructInst
	{
		std::string type;
		uint64_t off;
		std::string notes;
	};

	// data
	std::unordered_map<std::string, Struct*> structs;
	std::vector<StructInst> instances;

	// ui state
	int editMode = 0;
	uint32_t curInst = 0;
	uint32_t curField = 0;

	uint64_t GetFixedFieldSize(const Field& field)
	{
		auto it = g_builtinTypes.find(field.type);
		if (it != g_builtinTypes.end())
			return it->second.size;
		auto sit = structs.find(field.type);
		if (sit != structs.end() && !sit->second->serialized)
			return sit->second->size;
		return UINT64_MAX;
	}

	struct ReadField
	{
		std::string preview;
		uint64_t off;
	};
	void ReadBuiltinFieldPreview(IDataSource* ds, uint64_t off, uint64_t count, const BuiltinTypeInfo& BTI, std::string& outPreview)
	{
		char bfr[8];
		for (int i = 0; i < std::min(count, 16ULL); i++)
		{
			if (BTI.separated && i > 0)
				outPreview += ", ";
			ds->Read(off, BTI.size, bfr);
			BTI.append_to_str(bfr, outPreview);
			off += BTI.size;
		}
		if (count > 16)
		{
			outPreview += "...";
		}
	}
	void ReadStruct(IDataSource* ds, const Struct& S, uint64_t off, std::vector<ReadField>& out)
	{
		if (S.serialized)
		{
			for (const auto& F : S.fields)
			{
				ReadField rf = { "", off };
				auto it = g_builtinTypes.find(F.type);
				if (it != g_builtinTypes.end())
				{
					ReadBuiltinFieldPreview(ds, off, F.count, it->second, rf.preview);
					off += it->second.size * F.count;
				}
				out.push_back(rf);
			}
		}
		else
		{
			for (const auto& F : S.fields)
			{
				ReadField rf = { "", off };
				auto it = g_builtinTypes.find(F.type);
				if (it != g_builtinTypes.end())
				{
					ReadBuiltinFieldPreview(ds, off + F.off, F.count, it->second, rf.preview);
				}
				out.push_back(rf);
			}
		}
	}

	void BRB(UIContainer* ctx, const char* text, int& at, int val)
	{
		auto* rb = ctx->MakeWithText<ui::RadioButtonT<int>>(text);
		rb->Init(at, val);
		rb->onChange = [rb]() { rb->RerenderNode(); };
		rb->SetStyle(ui::Theme::current->button);
	}
	void Edit(UIContainer* ctx, const char* path)
	{
		auto* all = ctx->Push<ui::Panel>();

		ui::imm::PropEditInt(ctx, "Current instance ID", curInst, {}, 1, 0, instances.empty() ? 0 : instances.size() - 1);
		ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);
		ctx->Text("Edit:") + ui::Padding(5);
		BRB(ctx, "instance", editMode, 0);
		BRB(ctx, "struct", editMode, 1);
		BRB(ctx, "field", editMode, 2);
		ctx->Pop();

		if (editMode == 0)
		{
			if (curInst < instances.size())
			{
				auto& I = instances[curInst];

				ui::imm::PropEditString(ctx, "Notes", I.notes.c_str(), [&I](const char* s) { I.notes = s; });
				ui::imm::PropEditInt(ctx, "Offset", I.off);
				if (ui::imm::PropButton(ctx, "Edit struct:", I.type.c_str()))
				{
					editMode = 1;
					all->RerenderNode();
				}

				auto it = structs.find(I.type);
				if (it != structs.end())
				{
					auto& S = *it->second;
					struct Data : ui::TableDataSource
					{
						size_t GetNumRows() override { return rfs.size(); }
						size_t GetNumCols() override { return 3; }
						std::string GetRowName(size_t row) override { return std::to_string(row + 1); }
						std::string GetColName(size_t col) override
						{
							if (col == 0) return "Name";
							if (col == 1) return "Type";
							if (col == 2) return "Preview";
							return "";
						}
						std::string GetText(size_t row, size_t col) override
						{
							switch (col)
							{
							case 0: return S->fields[row].name;
							case 1: return S->fields[row].type + "[" + std::to_string(S->fields[row].count) + "]";
							case 2: return rfs[row].preview;
							default: return "";
							}
						}

						Struct* S;
						std::vector<ReadField> rfs;
					};
					ctx->MakeWithText<ui::BoxElement>("Data");
					FileDataSource fds;
					Data data;
					data.S = &S;
					fds.fp = fopen(("FRET_Plugins/" + std::string(path)).c_str(), "rb");
					ReadStruct(&fds, S, I.off, data.rfs);
					fclose(fds.fp);
					for (size_t i = 0; i < S.fields.size(); i++)
					{
						auto& F = S.fields[i];
						char bfr[256];
						snprintf(bfr, 256, "%s: %s[%" PRIu64 "]=%s", F.name.c_str(), F.type.c_str(), F.count, data.rfs[i].preview.c_str());
						*ctx->MakeWithText<ui::BoxElement>(bfr) + ui::Padding(5);
					}
					/*auto* tv = ctx->Make<ui::TableView>();
					tv->SetDataSource(&data); TODO */
				}
			}
		}
		if (editMode == 1)
		{
			if (curInst < instances.size())
			{
				auto& I = instances[curInst];

				ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);
				ctx->Text("Struct:") + ui::Padding(5);
				ctx->Text(I.type.c_str()) + ui::Padding(5);
				ctx->Pop();

				auto it = structs.find(I.type);
				if (it != structs.end())
				{
					auto& S = *it->second;
					ui::imm::PropEditBool(ctx, "Is serialized?", S.serialized);
					ctx->Push<ui::Panel>();
					for (size_t i = 0; i < S.fields.size(); i++)
					{
						auto& F = S.fields[i];
						ctx->PushBox() + ui::Layout(style::layouts::StackExpand()) + ui::StackingDirection(style::StackingDirection::LeftToRight);
						//ctx->Text(F.name.c_str());
						//ctx->Text(F.type.c_str());
						//ui::imm::EditInt(ctx, "\bCount", F.count);
						char info[128];
						snprintf(info, 128, "%s: %s[%" PRIu64 "]", F.name.c_str(), F.type.c_str(), F.count);
						*ctx->MakeWithText<ui::BoxElement>(info) + ui::Padding(5);
						//ctx->MakeWithText<ui::Button>("Edit");
						if (i > 0 && ui::imm::Button(ctx, "<", { &ui::Width(20) }))
						{
							std::swap(S.fields[i - 1], S.fields[i]);
							all->RerenderNode();
						}
						if (i + 1 < S.fields.size() && ui::imm::Button(ctx, ">", { &ui::Width(20) }))
						{
							std::swap(S.fields[i + 1], S.fields[i]);
							all->RerenderNode();
						}
						if (ui::imm::Button(ctx, "Edit", { &ui::Width(50) }))
						{
							editMode = 2;
							curField = i;
							all->RerenderNode();
						}
						if (ui::imm::Button(ctx, "X", { &ui::Width(20) }))
						{
							S.fields.erase(S.fields.begin() + i);
							all->RerenderNode();
						}
						ctx->Pop();
					}
					if (ui::imm::Button(ctx, "Add"))
					{
						S.fields.push_back({ "i32", "unnamed", 1 });
						editMode = 2;
						curField = S.fields.size() - 1;
						all->RerenderNode();
					}
					ctx->Pop();
				}
				else
				{
					ctx->Text("-- NOT FOUND --") + ui::Padding(10);
				}
			}
		}
		if (editMode == 2)
		{
			if (curInst < instances.size())
			{
				auto& I = instances[curInst];

				auto it = structs.find(I.type);
				if (it != structs.end())
				{
					auto& S = *it->second;
					if (curField < S.fields.size())
					{
						auto& F = S.fields[curField];
						if (!S.serialized)
							ui::imm::PropEditInt(ctx, "Offset", F.off);
						ui::imm::PropEditString(ctx, "Name", F.name.c_str(), [&F](const char* s) { F.name = s; });
						ui::imm::PropEditString(ctx, "Type", F.type.c_str(), [&F](const char* s) { F.type = s; });
						ui::imm::PropEditInt(ctx, "Count", F.count, {}, 1, 1);
					}
				}
			}
		}

		ctx->Pop();
	}
};

struct HighlightSettings
{
	bool excludeZeroes = true;
	bool enableFloat32 = true;
	float minFloat32 = 0.0001f;
	float maxFloat32 = 10000;
	bool enableInt16 = true;
	int32_t minInt16 = -2000;
	int32_t maxInt16 = 2000;
	bool enableInt32 = true;
	int32_t minInt32 = -10000;
	int32_t maxInt32 = 10000;
	int minASCIIChars = 3;

	void EditUI(UIContainer* ctx)
	{
		ui::imm::PropEditBool(ctx, "Exclude zeroes", excludeZeroes);

		ui::Property::Begin(ctx, "float32");
		ui::imm::PropEditBool(ctx, nullptr, enableFloat32);
		ui::imm::PropEditFloat(ctx, "\bMin", minFloat32, {}, 0.01f);
		ui::imm::PropEditFloat(ctx, "\bMax", maxFloat32);
		ui::Property::End(ctx);

		ui::Property::Begin(ctx, "int16");
		ui::imm::PropEditBool(ctx, nullptr, enableInt16);
		ui::imm::PropEditInt(ctx, "\bMin", minInt16);
		ui::imm::PropEditInt(ctx, "\bMax", maxInt16);
		ui::Property::End(ctx);

		ui::Property::Begin(ctx, "int32");
		ui::imm::PropEditBool(ctx, nullptr, enableInt32);
		ui::imm::PropEditInt(ctx, "\bMin", minInt32);
		ui::imm::PropEditInt(ctx, "\bMax", maxInt32);
		ui::Property::End(ctx);

		ui::Property::Begin(ctx, "ASCII");
		ui::imm::PropEditInt(ctx, "\bMin chars", minASCIIChars, {}, 1, 1, 128);
		ui::Property::End(ctx);
	}
};

struct REFile
{
	REFile(std::string n)
	{
		path = n;
		name = n;
		//ds = new FileStructureDataSource(n.c_str());
	}
	~REFile()
	{
		//delete ds;
	}

	std::string path;
	std::string name;
	FileStructureDataSource* ds;
	MarkerData mdata;
	uint32_t byteWidth = 16;
	HighlightSettings highlightSettings;
	DataDesc desc;
};

struct HexViewer : UIElement, HighlightSettings
{
	HexViewer()
	{
		GetStyle().SetWidth(style::Coord::Percent(100));
		GetStyle().SetHeight(style::Coord::Percent(100));
	}
	~HexViewer()
	{
		if (fp)
			fclose(fp);
	}
	void OnEvent(UIEvent& e) override
	{
		int W = refile->byteWidth;

		if (e.type == UIEventType::MouseMove)
		{
			float fh = GetFontHeight() + 4;
			float x = finalRectC.x0 + 2;
			float y = finalRectC.y0;
			float x2 = x + 20 * W + 10;

			hoverSection = -1;
			hoverByte = UINT64_MAX;
			if (e.x >= x && e.x < x + W * 20)
			{
				hoverSection = 0;
				int xpos = std::min(std::max(0, int((e.x - x) / 20)), W - 1);
				int ypos = (e.y - y) / fh;
				hoverByte = GetBasePos() + xpos + ypos * 16;
			}
			else if (e.x >= x2 && e.x < x2 + W * 10)
			{
				hoverSection = 1;
				int xpos = std::min(std::max(0, int((e.x - x2) / 10)), W - 1);
				int ypos = (e.y - y) / fh;
				hoverByte = GetBasePos() + xpos + ypos * 16;
			}
		}
		else if (e.type == UIEventType::MouseLeave)
		{
			hoverSection = -1;
			hoverByte = UINT64_MAX;
		}
		else if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Right)
		{
			uint64_t pos = hoverByte;

			char txt_pos[64];
			snprintf(txt_pos, 32, "@ %" PRIu64 " (0x%" PRIX64 ")", pos, pos);

			char txt_int16[32];
			GetInt16Text(txt_int16, 32, pos, true);
			char txt_uint16[32];
			GetInt16Text(txt_uint16, 32, pos, false);
			char txt_int32[32];
			GetInt32Text(txt_int32, 32, pos, true);
			char txt_uint32[32];
			GetInt32Text(txt_uint32, 32, pos, false);
			char txt_float32[32];
			GetFloat32Text(txt_float32, 32, pos);

			ui::MenuItem items[] =
			{
				ui::MenuItem(txt_pos, {}, true),
				ui::MenuItem("Mark int16", txt_int16).Func([this, pos]() { refile->mdata.AddMarker(DT_I16, pos, pos + 2); }),
				ui::MenuItem("Mark uint16", txt_uint16).Func([this, pos]() { refile->mdata.AddMarker(DT_U16, pos, pos + 2); }),
				ui::MenuItem("Mark int32", txt_int32).Func([this, pos]() { refile->mdata.AddMarker(DT_I32, pos, pos + 4); }),
				ui::MenuItem("Mark uint32", txt_uint32).Func([this, pos]() { refile->mdata.AddMarker(DT_U32, pos, pos + 4); }),
				ui::MenuItem("Mark float32", txt_float32).Func([this, pos]() { refile->mdata.AddMarker(DT_F32, pos, pos + 4); }),
			};
			ui::Menu menu(items);
			menu.Show(this);
		}
		else if (e.type == UIEventType::MouseScroll)
		{
			int64_t diff = round(e.dy / 40) * 16;
			if (diff > 0 && diff > basePos)
				basePos = 0;
			else
				basePos -= diff;
			basePos = std::min(GetFileLength() - 1, basePos);
		}
	}
	void OnPaint() override
	{
		int W = refile->byteWidth;

		uint8_t buf[256 * 64];
		size_t sz = 0;
		if (fp)
		{
			fseek(fp, GetBasePos(), SEEK_SET);
			sz = fread(buf, 1, W * 64, fp);
		}

		float fh = GetFontHeight() + 4;
		float x = finalRectC.x0 + 2;
		float y = finalRectC.y0 + fh;
		float x2 = x + 20 * W + 10;

		GL::SetTexture(0);
		GL::BatchRenderer br;
		br.Begin();
		for (size_t i = 0; i < sz; i++)
		{
			uint8_t v = buf[i];
			Color4f col = GetByteTypeBin(buf, i, sz);
			auto mc = refile->mdata.GetMarkedColor(GetBasePos() + i);
			if (hoverByte == GetBasePos() + i)
				col.BlendOver(colorHover);
			if (mc.a > 0)
				col.BlendOver(mc);
			if (col.a > 0)
			{
				float xoff = (i % W) * 20;
				float yoff = (i / W) * fh;
				br.SetColor(col.r, col.g, col.b, col.a);
				br.Quad(x + xoff - 2, y + yoff - fh + 4, x + xoff + 18, y + yoff + 3, 0, 0, 1, 1);
			}
			col = GetByteTypeASCII(buf, i, sz);
			if (hoverByte == GetBasePos() + i)
				col.BlendOver(colorHover);
			if (mc.a > 0)
				col.BlendOver(mc);
			if (col.a > 0)
			{
				float xoff = (i % W) * 10;
				float yoff = (i / W) * fh;
				br.SetColor(col.r, col.g, col.b, col.a);
				br.Quad(x2 + xoff - 2, y + yoff - fh + 4, x2 + xoff + 8, y + yoff + 3, 0, 0, 1, 1);
			}
		}
		br.End();

		for (size_t i = 0; i < sz; i++)
		{
			uint8_t v = buf[i];
			char str[3];
			str[0] = "0123456789ABCDEF"[v >> 4];
			str[1] = "0123456789ABCDEF"[v & 0xf];
			str[2] = 0;
			float xoff = (i % W) * 20;
			float yoff = (i / W) * fh;
			DrawTextLine(x + xoff, y + yoff, str, 1, 1, 1);
			str[1] = IsASCII(v) ? v : '.';
			DrawTextLine(x2 + xoff / 2, y + yoff, str + 1, 1, 1, 1);
		}
	}
	void OnSerialize(IDataSerializer& s) override
	{
		s << basePos;
	}

	uint64_t basePos = 16 * 3800 * 0;
	uint64_t GetBasePos()
	{
		return basePos;
	}

	void Init(REFile* rf)
	{
		refile = rf;
		if (fp)
			fclose(fp);
		fp = fopen(("FRET_Plugins/" + rf->path).c_str(), "rb");
	}

	Color4f GetByteTypeBin(uint8_t* buf, int at, int sz)
	{
		Color4f col(0);
		if (IsInt16InRange(buf, at, sz)) col.BlendOver(colorInt16);
		if (IsInt16InRange(buf, at - 1, sz)) col.BlendOver(colorInt16);
		if (IsFloat32InRange(buf, at, sz)) col.BlendOver(colorFloat32);
		if (IsFloat32InRange(buf, at - 1, sz)) col.BlendOver(colorFloat32);
		if (IsFloat32InRange(buf, at - 2, sz)) col.BlendOver(colorFloat32);
		if (IsFloat32InRange(buf, at - 3, sz)) col.BlendOver(colorFloat32);
		if (IsInt32InRange(buf, at, sz)) col.BlendOver(colorInt32);
		if (IsInt32InRange(buf, at - 1, sz)) col.BlendOver(colorInt32);
		if (IsInt32InRange(buf, at - 2, sz)) col.BlendOver(colorInt32);
		if (IsInt32InRange(buf, at - 3, sz)) col.BlendOver(colorInt32);
		return col;
	}
	Color4f GetByteTypeASCII(uint8_t* buf, int at, int sz)
	{
		Color4f col(0);
		if (IsASCIIInRange(buf, at, sz)) col.BlendOver(colorASCII);
		return col;
	}

	bool IsInt16InRange(uint8_t* buf, int at, int sz)
	{
		if (!enableInt16 || at < 0 || sz - at < 2)
			return false;
		if (refile->mdata.IsMarked(GetBasePos() + at, 2))
			return false;
		if (excludeZeroes && buf[at] == 0 && buf[at + 1] == 0)
			return false;
		int16_t v;
		memcpy(&v, &buf[at], 2);
		return v >= minInt16 && v <= maxInt16;
	}
	bool IsInt32InRange(uint8_t* buf, int at, int sz)
	{
		if (!enableInt32 || at < 0 || sz - at < 4)
			return false;
		if (refile->mdata.IsMarked(GetBasePos() + at, 4))
			return false;
		if (excludeZeroes && buf[at] == 0 && buf[at + 1] == 0 && buf[at + 2] == 0 && buf[at + 3] == 0)
			return false;
		int32_t v;
		memcpy(&v, &buf[at], 4);
		return v >= minInt32 && v <= maxInt32;
	}
	bool IsFloat32InRange(uint8_t* buf, int at, int sz)
	{
		if (!enableFloat32 || at < 0 || sz - at < 4)
			return false;
		if (refile->mdata.IsMarked(GetBasePos() + at, 4))
			return false;
		if (excludeZeroes && buf[at] == 0 && buf[at + 1] == 0 && buf[at + 2] == 0 && buf[at + 3] == 0)
			return false;
		float v;
		memcpy(&v, &buf[at], 4);
		return (v >= minFloat32 && v <= maxFloat32) || (v >= -maxFloat32 && v <= -minFloat32);
	}
	bool IsASCIIInRange(uint8_t* buf, int at, int sz)
	{
		if (!IsASCII(buf[at]))
			return false;
		int min = at;
		int max = at;
		while (min - 1 >= 0 && max - min < minASCIIChars && IsASCII(buf[min - 1]))
			min--;
		while (max + 1 < sz && max - min < minASCIIChars && IsASCII(buf[max + 1]))
			max++;
		return max - min >= minASCIIChars;
	}
	static bool IsASCII(uint8_t v)
	{
		return v >= 0x20 && v < 0x7f;
	}
	uint64_t GetFileLength()
	{
		if (!fp)
			return 0;
		fseek(fp, 0, SEEK_END);
		return _ftelli64(fp);
	}

	void GetInt16Text(char* buf, size_t bufsz, uint64_t pos, bool sign)
	{
		int16_t v;
		size_t sz = 0;
		if (!fp || fseek(fp, pos, SEEK_SET) || !fread(&v, 2, 1, fp))
		{
			strncpy(buf, "-", bufsz);
			return;
		}
		snprintf(buf, bufsz, sign ? "%" PRId16 : "%" PRIu16, v);
	}
	void GetInt32Text(char* buf, size_t bufsz, uint64_t pos, bool sign)
	{
		int32_t v;
		size_t sz = 0;
		if (!fp || fseek(fp, pos, SEEK_SET) || !fread(&v, 4, 1, fp))
		{
			strncpy(buf, "-", bufsz);
			return;
		}
		snprintf(buf, bufsz, sign ? "%" PRId32 : "%" PRIu32, v);
	}
	void GetFloat32Text(char* buf, size_t bufsz, uint64_t pos)
	{
		float v;
		size_t sz = 0;
		if (!fp || fseek(fp, pos, SEEK_SET) || !fread(&v, 4, 1, fp))
		{
			strncpy(buf, "-", bufsz);
			return;
		}
		snprintf(buf, bufsz, "%g", v);
	}

	REFile* refile = nullptr;
	FILE* fp = nullptr;

	Color4f colorHover{ 1, 1, 1, 0.3f };
	uint64_t hoverByte = UINT64_MAX;
	int hoverSection = -1;
};

struct MarkedItemsList : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		Subscribe(DCT_MarkedItems, mdata);
		ctx->Text("Edit marked items");
		for (auto& m : mdata->markers)
		{
			ctx->Push<ui::Panel>();
			if (ui::imm::PropButton(ctx, "Type", "EDIT"))
			{
				std::vector<ui::MenuItem> items;
				items.push_back(ui::MenuItem("int32", {}, false, true));
				ui::Menu m(items);
				m.Show(this);
			}
			ui::imm::PropEditInt(ctx, "Offset", m.at);
			ui::imm::PropEditInt(ctx, "Count", m.count);
			ui::imm::PropEditInt(ctx, "Repeats", m.repeats);
			ui::imm::PropEditInt(ctx, "Stride", m.stride);
			ctx->Pop();
		}
	}

	MarkerData* mdata;
};

struct MainWindow : ui::NativeMainWindow
{
	MainWindow()
	{
		SetSize(1200, 800);
#if 1
		{
			auto* f = new REFile("loop.wav");
			auto* chunk = new DataDesc::Struct;
			chunk->serialized = true;
			chunk->fields.push_back({ "char", "type", 4 });
			chunk->fields.push_back({ "u32", "size", 1 });
			f->desc.structs["chunk"] = chunk;
			auto* fmt_data = new DataDesc::Struct;
			fmt_data->serialized = true;
			fmt_data->fields.push_back({ "u16", "AudioFormat", 1 });
			fmt_data->fields.push_back({ "u16", "NumChannels", 1 });
			fmt_data->fields.push_back({ "u32", "SampleRate", 1 });
			fmt_data->fields.push_back({ "u32", "ByteRate", 1 });
			fmt_data->fields.push_back({ "u16", "BlockAlign", 1 });
			fmt_data->fields.push_back({ "u16", "BitsPerSample", 1 });
			f->desc.structs["fmt_data"] = fmt_data;
			f->desc.instances.push_back({ "chunk", 0, "RIFF chunk" });
			f->desc.instances.push_back({ "fmt_data", 20, "fmt chunk data" });
			files.push_back(f);
		}
#endif
		//files.push_back(new REFile("tree.mesh"));
		//files.push_back(new REFile("arch.tar"));
	}
	void OnRender(UIContainer* ctx) override
	{
		auto s = ctx->Push<ui::TabGroup>()->GetStyle();
		s.SetLayout(style::layouts::EdgeSlice());
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
				{
					auto s = p->GetStyle();
					s.SetLayout(style::layouts::EdgeSlice());
					s.SetHeight(style::Coord::Percent(100));
					//s.SetBoxSizing(style::BoxSizing::BorderBox);
					*ctx->Push<ui::Panel>() + ui::BoxSizing(style::BoxSizing::BorderBox) + ui::Height(style::Coord::Percent(100));
					{
						//ctx->Make<FileStructureViewer2>()->ds = f->ds;
						auto* sp = ctx->Push<ui::SplitPane>();
						{
							ctx->PushBox() + ui::Layout(style::layouts::EdgeSlice());

							ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);
							auto* vs = ctx->MakeWithText<ui::CollapsibleTreeNode>("View settings");
							auto* hs = ctx->MakeWithText<ui::CollapsibleTreeNode>("Highlight settings");
							ctx->Pop();

							ctx->PushBox(); // tree stabilization box
							if (vs->open)
							{
								ui::imm::PropEditInt(ctx, "Width", f->byteWidth, {}, 1, 1, 256);
							}
							if (hs->open)
							{
								f->highlightSettings.EditUI(ctx);
							}
							ctx->Pop(); // end tree stabilization box

							auto* hv = ctx->Make<HexViewer>();
							hv->Init(f);
							*static_cast<HighlightSettings*>(hv) = f->highlightSettings;
							ctx->Pop();

							ctx->PushBox();
							ctx->Text("Marked items");
							ctx->Pop();

							ctx->PushBox();
							ctx->Make<MarkedItemsList>()->mdata = &f->mdata;
							f->desc.Edit(ctx, f->path.c_str());
							ctx->Pop();
						}
						sp->SetSplit(0, 0.45f);
						sp->SetSplit(1, 0.7f);
						ctx->Pop();
					}
					ctx->Pop();
				}
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
