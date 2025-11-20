
#pragma once

#include "ObjectIteration.h"
#include "Platform.h"
#include "String.h"


namespace ui {

struct GUID
{
	union
	{
		u64 v64[2];
		u32 v32[4];
		u8 v8[16];
	};

	UI_FORCEINLINE GUID() { v64[0] = 0; v64[1] = 0; }
	UI_FORCEINLINE GUID(DoNotInitialize) {}

	UI_FORCEINLINE bool IsNull() const { return v64[0] == 0 && v64[1] == 0; }
	UI_FORCEINLINE bool NotNull() const { return v64[0] != 0 || v64[1] != 0; }
	UI_FORCEINLINE bool operator == (const GUID& o) const { return v64[0] == o.v64[0] && v64[1] == o.v64[1]; }
	UI_FORCEINLINE bool operator != (const GUID& o) const { return v64[0] != o.v64[0] || v64[1] != o.v64[1]; }
	bool operator < (const GUID& o) const { return memcmp(v8, o.v8, 16) < 0; }
	bool operator <= (const GUID& o) const { return memcmp(v8, o.v8, 16) <= 0; }
	bool operator > (const GUID& o) const { return memcmp(v8, o.v8, 16) > 0; }
	bool operator >= (const GUID& o) const { return memcmp(v8, o.v8, 16) >= 0; }

	bool OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		bool un = oi.IsUnserializer();
		std::string tmp;
		if (!un)
			tmp = ToStringHex();
		bool ret = OnField(oi, FI, tmp);
		if (un)
		{
			ret &= ValidateStringHex(tmp);
			*this = FromStringHex(tmp);
		}
		return ret;
	}

	UI_FORCEINLINE static GUID Null() { return {}; }
	static GUID New();
	static bool ValidateStringHex(StringView str);
	static bool ValidateStringHexSplit(StringView str);
	static GUID FromStringHex(StringView str);
	static GUID FromStringHexSplit(StringView str);

	void ToStringHex(char out[32], bool upper = false) const;
	void ToStringHexSplit(char out[36], bool upper = false) const;
	std::string ToStringHex(bool upper = false) const;
	std::string ToStringHexSplit(bool upper = false) const;
};

inline size_t HashValue(const GUID& guid)
{
	size_t h = guid.v32[0];
	h *= 131;
	h ^= guid.v32[1];
	h *= 131;
	h ^= guid.v32[2];
	h *= 131;
	h ^= guid.v32[3];
	return h;
}

} // ui
