
#include "plugin.hpp"

struct WAVReader : Reader
{
	void Parse(const char*)
	{
		ReadChunk();
	}
	
	void ReadChunk(bool isinfo = false)
	{
		PUSH("chunk");
		READA(Magic, char, 4);
		READ(Size, u32);
		int end = at + Size;
		if (memcmp(Magic, "RIFF", 4) == 0 || memcmp(Magic, "LIST", 4) == 0)
		{
			READA(ListMagic, char, 4);
			isinfo = memcmp(ListMagic, "INFO", 4) == 0;
			while (at < end)
			{
				ReadChunk(isinfo);
			}
		}
		else if (memcmp(Magic, "fmt ", 4) == 0)
		{
			READ(AudioFormat, u16);
			READ(NumChannels, u16);
			READ(SampleRate, u32);
			READ(ByteRate, u32);
			READ(BlockAlign, u16);
			READ(BitsPerSample, u16);
		}
		else if (isinfo)
		{
			MARKA(text, char, Size);
		}
		at = end;
		POP();
	}
};

DEFINE_PLUGIN(WAVReader);
