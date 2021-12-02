
#pragma once
#include "Platform.h"


namespace ui {

struct StaticIDStorage;

StaticIDStorage* StaticIDStorage_Create();
uint32_t StaticIDStorage_Alloc(StaticIDStorage* storage, const char* name);
uint32_t StaticIDStorage_GetCount(StaticIDStorage* storage);

template <class T>
struct StaticID
{
	const char* _name;
	uint32_t _id;

	UI_FORCEINLINE explicit StaticID(const char* name) : _name(name)
	{
		_id = StaticIDStorage_Alloc(GetStorage(), name);
	}
	UI_FORCEINLINE bool operator == (const StaticID& o) const { return _id == o._id; }
	UI_FORCEINLINE bool operator != (const StaticID& o) const { return _id != o._id; }

	static StaticIDStorage* GetStorage()
	{
		static StaticIDStorage* storage = StaticIDStorage_Create();
		return storage;
	}
	static uint32_t GetCount()
	{
		return StaticIDStorage_GetCount(GetStorage());
	}
};

} // ui
