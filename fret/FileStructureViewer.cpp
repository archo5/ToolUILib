
#include "pch.h"
#include "FileStructureViewer.h"


uint64_t ISyncReadStream::ReadULEB()
{
	uint64_t result = 0;
	int shift = 0;
	for (;;)
	{
		uint8_t b;
		if (!Read(&b, 1))
			return 0;
		result |= uint64_t(b & 0x7f) << shift;
		if (!(b & 0x80))
			break;
		shift += 7;
	}
	return result;
}


SocketSyncReadStream::SocketSyncReadStream(int port)
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

SocketSyncReadStream::~SocketSyncReadStream()
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

void SocketSyncReadStream::Accept()
{
	sockaddr_storage oaddr;
	int addr_size = sizeof(oaddr);
	rs = ::accept(s, (sockaddr*)&oaddr, &addr_size);
}

int SocketSyncReadStream::Read(void* buf, int size)
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


char StructureParser::ReadNext(ISyncReadStream* s)
{
	char type;
	if (!s->Read(&type, 1))
		return 0;

	if (type == '{')
	{
		name.resize((size_t)s->ReadULEB());
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
		fileOffset = s->ReadULEB();
		arraySize = s->ReadULEB();

		name.resize((size_t)s->ReadULEB());
		if (s->Read(&name[0], name.size()) != name.size())
			return 0; // failed to read the whole string

		preview.resize((size_t)s->ReadULEB());
		if (s->Read(&preview[0], preview.size()) != preview.size())
			return 0; // failed to read the whole string
	}
	else if (type == 'I')
	{
		name.resize((size_t)s->ReadULEB());
		if (s->Read(&name[0], name.size()) != name.size())
			return 0; // failed to read the whole string

		preview.resize((size_t)s->ReadULEB());
		if (s->Read(&preview[0], preview.size()) != preview.size())
			return 0; // failed to read the whole string
	}
	// no data for }
	return type;
}


void FileStructureViewer::Build()
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
#if 0 // CLEAN THIS UP
				auto& item = ui::Push<ui::CollapsibleTreeNode>();
				if (ui::LastIsNew())
					item.open = openness[id];
				else
					openness[id] = item.open;

				if (item.open)
					openLevel++;
#endif
				ui::Text(sp.name.c_str());
			}
			curLevel++;
			break;
		}
		case '}':
			if (curLevel - 1 == openLevel)
			{
				ui::Pop();
			}
			else if (curLevel == openLevel)
			{
				ui::Pop();
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
				ui::Text(text.str().c_str());
			}
			break;
		}
	}
}


void FileStructureDataSource::Parse(ISyncReadStream* srs, StructureParser& sp, Node* parent)
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
