
#include "FileSystem.h"


namespace ui {

std::string ReadTextFile(const char* path)
{
	FILE* f = fopen(path, "r");
	if (!f)
		return {};
	std::string data;
	fseek(f, 0, SEEK_END);
	if (auto s = ftell(f))
	{
		data.resize(s);
		fseek(f, 0, SEEK_SET);
		s = fread(&data[0], 1, s, f);
		data.resize(s);
	}
	fclose(f);
	return data;
}

bool WriteTextFile(const char* path, StringView text)
{
	FILE* f = fopen(path, "w");
	if (!f)
		return false;
	bool success = fwrite(text.data(), text.size(), 1, f) != 0;
	fclose(f);
	return success;
}

} // ui
