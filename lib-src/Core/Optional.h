
#pragma once

#include "Common.h"
#include "ObjectIterationCore.h"


namespace ui {

template <class T>
struct Optional
{
	T _value{};
	bool _hasValue = false;

	UI_FORCEINLINE Optional() {}
	UI_FORCEINLINE Optional(const T& v, bool hasValue = true) : _value(v), _hasValue(hasValue) {}

	UI_FORCEINLINE bool operator == (const Optional& o) const
	{
		return _hasValue == o._hasValue && (_hasValue ? _value == o._value : true);
	}
	UI_FORCEINLINE bool operator != (const Optional& o) const { return !(*this == o); }
	UI_FORCEINLINE operator void*() const { return (void*)_hasValue; }

	UI_FORCEINLINE bool HasValue() const
	{
		return _hasValue;
	}
	UI_FORCEINLINE const T& GetValue() const
	{
		assert(_hasValue);
		return _value;
	}
	UI_FORCEINLINE const T* GetValuePtrOrNull() const
	{
		return _hasValue ? &_value : nullptr;
	}
	UI_FORCEINLINE T GetValueOrDefault(const T& def) const
	{
		return _hasValue ? _value : def;
	}
	UI_FORCEINLINE Optional<T> Or(const Optional<T>& next) const
	{
		return _hasValue ? *this : next;
	}
	UI_FORCEINLINE void SetValue(const T& v)
	{
		_value = v;
		_hasValue = true;
	}
	UI_FORCEINLINE void ClearValue()
	{
		_value = {};
		_hasValue = false;
	}
	template <class R>
	UI_FORCEINLINE Optional<R> StaticCast() const
	{
		if (_hasValue)
			return static_cast<R>(_value);
		return {};
	}

	template <class Func>
	void OnSerializeCustom(IObjectIterator& oi, const FieldInfo& FI, Func&& func)
	{
		oi.BeginObject(FI, "Optional<T>");
		OnField(oi, "hasValue", _hasValue);
		func(oi, "value", _value);
		oi.EndObject();
	}
	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		oi.BeginObject(FI, "Optional<T>");
		OnField(oi, "hasValue", _hasValue);
		OnField(oi, "value", _value);
		oi.EndObject();
	}
};

} // ui
