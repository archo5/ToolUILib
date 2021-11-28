
#include "StaticID.h"

#include "String.h"
#include "HashTable.h"


namespace ui {

struct StaticIDStorage
{
	uint32_t count = 0;
	HashMap<StringView, uint32_t> ids;

	uint32_t Alloc(const char* name)
	{
		auto it = ids.find(name);
		if (it.is_valid())
			return it->value;

		uint32_t id = count++;
		ids.insert(name, id);
		return id;
	}
};

StaticIDStorage* StaticIDStorage_Create()
{
	return new StaticIDStorage;
}

uint32_t StaticIDStorage_Alloc(StaticIDStorage* storage, const char* name)
{
	return storage->Alloc(name);
}

uint32_t StaticIDStorage_GetCount(StaticIDStorage* storage)
{
	return storage->count;
}

} // ui
