
#include "plugin.hpp"

static u8 dummyzeroes[1024];

struct TARReader : Reader
{
	void Parse(const char*)
	{
		while (at < size_)
		{
			if (size_ - at >= 1024 && memcmp(&data_[at], dummyzeroes, 1024) == 0)
			{
				INFO("end of file marker", "(1024 zero bytes)");
				at += 1024;
				continue;
			}

			PUSH("file");

			MARKA(Name, char, 100);
			MARKA(Mode, char, 8);
			MARKA(Owner, char, 8);
			MARKA(Group, char, 8);
			READA(FileSizeOctal, char, 12);
			u64 FileSize = ParseOctal(FileSizeOctal, 12);
			PARSED(FileSize, FileSize);
			READA(LastModTimeOctal, char, 12);
			PARSED(LastModTime, ParseOctal(LastModTimeOctal, 12));
			READA(Checksum, char, 8);
			READ(Type, char);
			MARKA(LinkName, char, 100);
			MARKA(UStarIndicator, char, 6);
			MARKA(UStarVersion, char, 2);
			MARKA(OwnerUserName, char, 32);
			MARKA(OwnerGroupName, char, 32);
			MARKA(DeviceMajorNumber, char, 8);
			MARKA(DeviceMinorNumber, char, 8);
			MARKA(FilenamePrefix, char, 155);

			align_up(at, 512);
			FILE_PLAIN(Data, FileSize);
			//at += FileSize;
			align_up(at, 512);

			POP();
		}
	}

	static u64 ParseOctal(const char* str, size_t maxsz)
	{
		u64 ret = 0;
		for (size_t i = 0; i < maxsz && str[i]; i++)
		{
			ret *= 8;
			ret += str[i] - '0';
		}
		return ret;
	}
};

DEFINE_PLUGIN(TARReader);
