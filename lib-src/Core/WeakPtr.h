
#pragma once

#include "Threading.h"


namespace ui {

struct LivenessToken
{
	struct Data
	{
		AtomicInt32 ref;
		AtomicBool alive;
	};

	Data* _data;

	LivenessToken() : _data(nullptr) {}
	LivenessToken(Data* d) : _data(d) { if (d) d->ref++; }
	LivenessToken(const LivenessToken& o) : _data(o._data) { if (_data) _data->ref++; }
	LivenessToken(LivenessToken&& o) : _data(o._data) { o._data = nullptr; }
	~LivenessToken() { Release(); }
	LivenessToken& operator = (const LivenessToken& o)
	{
		Release();
		_data = o._data;
		if (_data)
			_data->ref++;
		return *this;
	}
	LivenessToken& operator = (LivenessToken&& o)
	{
		Release();
		_data = o._data;
		o._data = nullptr;
		return *this;
	}
	void Release()
	{
		if (_data && --_data->ref <= 0)
		{
			delete _data;
			_data = nullptr;
		}
	}

	bool IsAlive() const
	{
		return _data && _data->alive;
	}
	void SetAlive(bool alive)
	{
		if (_data)
			_data->alive = alive;
	}
	LivenessToken& GetOrCreate()
	{
		if (!_data)
			_data = new Data{ 1, true };
		return *this;
	}
};

struct LivenessTokenOwner : LivenessToken
{
	~LivenessTokenOwner()
	{
		SetAlive(false);
	}
};

#define UI_DECLARE_WEAK_PTR_COMPATIBLE \
	::ui::LivenessTokenOwner _livenessToken; \
	::ui::LivenessToken& GetLivenessToken() { return _livenessToken.GetOrCreate(); }

template <class T>
struct WeakPtr
{
	LivenessToken _token;
	T* _obj = nullptr;

	UI_FORCEINLINE WeakPtr() {}
	UI_FORCEINLINE WeakPtr(T* o) : _obj(o) { if (o) _token = o->GetLivenessToken(); }
	UI_FORCEINLINE T* Get() const { return _token.IsAlive() ? _obj : nullptr; }
	UI_FORCEINLINE T* operator -> () const { return Get(); }
	UI_FORCEINLINE operator T* () const { return Get(); }
};

} // ui
