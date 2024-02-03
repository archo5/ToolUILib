
#include "TweakableValue.h"

#include "FileSystem.h"
#include "HashMap.h"


namespace ui {

struct TweakValFile
{
	struct Num
	{
		double number;
		StringView uid;
	};

	u64 lastModTime = 0;
	FileReadResult frr;
	HashMap<int, Array<Num>> numberVals;

	StringView FindLine(int line)
	{
		if (!frr.data)
			return {};

		StringView data = frr.data->GetStringView();
		for (int i = 1; i < line; i++)
		{
			data = data.AfterFirst("\n");
		}
		data = data.UntilFirst("\n");
		return data;
	}

	double GetNumber(int line, const char* macroNamePfx, double original, const char* uid)
	{
		StringView uidSV = uid ? StringView(uid) : StringView();
		if (auto* ptr = numberVals.GetValuePtr(line))
		{
			for (auto& N : *ptr)
				if (N.uid == uidSV)
					return N.number;
			return original;
		}

		auto& vals = numberVals[line];
		StringView lineSV = FindLine(line);
		StringView macroPfxSV = macroNamePfx;
		if (lineSV.NotEmpty())
		{
			size_t at = lineSV.FindFirstAt(macroPfxSV);
			while (at < lineSV.Size())
			{
				auto after = lineSV.substr(at).AfterFirst("(");
				auto content = after.UntilFirst(")"); // TODO support nested parentheses?

				Num N;
				if (content.starts_with("0x"))
					N.number = content.take_int32();
				else
					N.number = content.take_float64();
				N.uid = content.AfterLast(",").AfterFirst("\"").UntilLast("\"");
				vals.Append(N);

				at = lineSV.FindFirstAt(macroPfxSV, at + macroPfxSV.Size());
			}
		}

		for (auto& N : vals)
			if (N.uid == uidSV)
				return N.number;
		return original;
	}
};

struct TweakValFileHEC
{
	UI_FORCEINLINE static bool AreEqual(const char* a, const char* b)
	{
		return a == b || strcmp(a, b) == 0;
	}
	UI_FORCEINLINE static size_t GetHash(const char* v)
	{
		return HashValue(v);
	}
};

static HashMap<const char*, TweakValFile*, TweakValFileHEC> g_tweakValFile;
static u64 g_tweakValLastUpdate = 0;
static int g_tweakValMinReloadIntervalMS = -1;

void TweakableValueSetMinReloadInterval(int ms)
{
	g_tweakValMinReloadIntervalMS = ms;
	TweakableValueForceSubsequentReloads();
}

void TweakableValueForceSubsequentReloads()
{
	for (auto& kvp : g_tweakValFile)
	{
		u64 newModTime = ui::GetFileModTimeUnixMS(kvp.key);
		if (kvp.value->lastModTime != newModTime)
		{
			kvp.value->lastModTime = newModTime;
			kvp.value->frr = ui::ReadTextFile(kvp.key);
			kvp.value->numberVals.Clear();
		}
	}
	g_tweakValLastUpdate = ui::GetTimeUnixMS();
}

static TweakValFile* GetTweakValFile(const char* file)
{
	if (g_tweakValMinReloadIntervalMS >= 0 &&
		ui::GetTimeUnixMS() > g_tweakValLastUpdate + g_tweakValMinReloadIntervalMS)
	{
		TweakableValueForceSubsequentReloads();
	}

	auto* tvf = g_tweakValFile.GetValueOrDefault(file);
	if (!tvf)
	{
		tvf = new TweakValFile;
		tvf->frr = ui::ReadTextFile(file);
		tvf->lastModTime = ui::GetFileModTimeUnixMS(file);
		g_tweakValFile.Insert(file, tvf);
	}
	return tvf;
}

double TweakableValue(const char* file, int line, const char* macroNamePfx, double original, const char* uid)
{
	if (auto* twf = GetTweakValFile(file))
	{
		return twf->GetNumber(line, macroNamePfx, original, uid);
	}
	return original;
}

} // ui
