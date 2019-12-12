
#pragma once

#include <vector>
#include <string>


namespace prs {

class PropertyStore;

struct PropertyValueID
{
	uint32_t type : 4;
	uint32_t unit : 4;
	uint32_t id : 24;
};

static_assert(sizeof(PropertyValueID) == 4, "incorrect size of PropertyID");

enum class TypeID
{
	Null,
	Enum,
	Float,
};

enum class Unit
{
	Pixels,
	Percent,
};

class FloatPropertyValuePool
{
public:
	struct Value
	{
		union
		{
			float value;
			uint32_t nextFree;
		};
	};

	PropertyValueID Alloc(Unit unit);
	void Free(PropertyValueID id);
	float Get(PropertyValueID id);
	void Set(PropertyValueID id, float v);

private:
	std::vector<Value> values;
	uint32_t firstFree = UINT32_MAX;
};

class PropertyBlock
{
public:
	~PropertyBlock();

	uint32_t _FindPropertySlot(const char* name);
	uint32_t _FindOrAllocPropertySlot(const char* name);

	bool GetFloat(const char* name, float& out) { Unit tmp; return GetFloat(name, out, tmp); }
	bool GetFloat(const char* name, float& out, Unit& unit);
	void SetFloat(const char* name, float value, Unit unit);

	template <class T>
	bool IsEnum(const char* name, typename T::_Value val)
	{
		typename T::_Value comp;
		return GetEnum<T>(name, comp) && comp == val;
	}
	template <class T>
	bool GetEnum(const char* name, typename T::_Value& out)
	{
		auto slot = _FindPropertySlot(name);
		if (slot == UINT32_MAX)
			return false;
		auto id = propValues[slot];
		if (id.type != uint32_t(TypeID::Enum) || (id.id >> 12) != T::_ID)
			return false;
		out = T::_Value(id.id & ((1 << 12) - 1));
		return true;
	}
	template <class T>
	void SetEnum(const char* name, typename T::_Value val)
	{
		auto slot = _FindOrAllocPropertySlot(name);
		auto& prop = propValues[slot];
		store->FreeProperty(prop);
		prop = { uint32_t(TypeID::Enum), 0, uint32_t((T::_ID << 12) | val) };
	}

	PropertyStore* store;
	std::vector<std::string> propNames;
	std::vector<PropertyValueID> propValues;
};

class PropertyStore
{
public:
	PropertyBlock* AllocBlock();
	void FreeProperty(PropertyValueID id);

	FloatPropertyValuePool floatPropValues;
};

}
