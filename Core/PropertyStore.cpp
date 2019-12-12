
#include "PropertyStore.h"

namespace prs {

PropertyValueID FloatPropertyValuePool::Alloc(Unit unit)
{
	uint32_t at;
	if (firstFree != UINT32_MAX)
	{
		at = firstFree;
		firstFree = values[at].nextFree;
	}
	else
	{
		at = values.size();
		Value v;
		v.value = 0;
		values.push_back(v);
	}
	values[at].value = 0;
	return { (uint32_t)TypeID::Float, (uint32_t)unit, at };
}

void FloatPropertyValuePool::Free(PropertyValueID id)
{
	values[id.id].nextFree = firstFree;
	firstFree = id.id;
}

float FloatPropertyValuePool::Get(PropertyValueID id)
{
	return values[id.id].value;
}

void FloatPropertyValuePool::Set(PropertyValueID id, float v)
{
	values[id.id].value = v;
}


PropertyBlock::~PropertyBlock()
{
	for (const auto& v : propValues)
		store->FreeProperty(v);
}

uint32_t PropertyBlock::_FindPropertySlot(const char* name)
{
	for (size_t i = 0; i < propNames.size(); i++)
		if (propNames[i] == name)
			return i;
	return UINT32_MAX;
}

uint32_t PropertyBlock::_FindOrAllocPropertySlot(const char* name)
{
	auto slot = _FindPropertySlot(name);
	if (slot == UINT32_MAX)
	{
		slot = propNames.size();
		propNames.push_back(name);
		propValues.push_back({ (uint32_t) TypeID::Null, 0, 0 });
	}
	return slot;
}

bool PropertyBlock::GetFloat(const char* name, float& out, Unit& unit)
{
	auto slot = _FindPropertySlot(name);
	auto prop = propValues[slot];
	if (slot == UINT32_MAX || prop.type != (uint32_t) TypeID::Float)
		return false;
	out = store->floatPropValues.Get(prop);
	unit = (Unit) prop.unit;
	return true;
}

void PropertyBlock::SetFloat(const char* name, float value, Unit unit)
{
	auto slot = _FindOrAllocPropertySlot(name);
	auto& prop = propValues[slot];
	if (prop.type != (uint32_t)TypeID::Float)
	{
		store->FreeProperty(prop);
		prop = store->floatPropValues.Alloc(unit);
	}
	else
	{
		prop.unit = (uint32_t)unit;
	}
	store->floatPropValues.Set(prop, value);
}


PropertyBlock* PropertyStore::AllocBlock()
{
	auto* b = new prs::PropertyBlock;
	b->store = this;
	return b;
}

void PropertyStore::FreeProperty(PropertyValueID id)
{
	switch ((TypeID) id.type)
	{
	case TypeID::Null:
		break;
	case TypeID::Float:
		floatPropValues.Free(id);
		break;
	}
}

}
