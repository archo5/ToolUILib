
#pragma once

#include "Platform.h"

#include <assert.h>


namespace ui {

typedef void MulticastDelegateGenericCallback(void* userdata);

template <class... Args>
struct MulticastDelegate
{
	struct Entry
	{
		MulticastDelegate* _owner;
		Entry* _prev;
		Entry* _next;

		virtual ~Entry() {}
		virtual void Call(Args...) = 0;
		void Destroy()
		{
			_owner->Remove(this);
		}
	};

	Entry* _first = nullptr;
	Entry* _last = nullptr;

	UI_FORCEINLINE MulticastDelegate() {}
	MulticastDelegate(const MulticastDelegate&) = delete;
	MulticastDelegate& operator = (const MulticastDelegate&) = delete;
	~MulticastDelegate()
	{
		Clear();
	}

	void Clear()
	{
		for (Entry* e = _first; e; )
		{
			Entry* ne = e->_next;
			delete e;
			e = ne;
		}
		_first = nullptr;
		_last = nullptr;
	}

	void _AddEntry(Entry* e)
	{
		e->_owner = this;
		e->_prev = _last;
		e->_next = nullptr;
		if (_last)
			_last->_next = e;
		else
			_first = e;
		_last = e;
	}

	template <class F>
	Entry* Add(F&& f)
	{
		struct FEntry : Entry
		{
			F fn;
			FEntry(F&& f) : fn(Move(f)) {}
			void Call(Args... args) override { fn(args...); }
		};
		auto* e = new FEntry(Move(f));
		_AddEntry(e);
		return e;
	}

	template <class F>
	Entry* AddNoArgs(F&& f)
	{
		struct FEntry : Entry
		{
			F fn;
			FEntry(F&& f) : fn(Move(f)) {}
			void Call(Args... args) override { fn(); }
		};
		auto* e = new FEntry(Move(f));
		_AddEntry(e);
		return e;
	}

	Entry* AddNoArgsUserData(MulticastDelegateGenericCallback* func, void* userdata)
	{
		struct GFPEntry : Entry
		{
			MulticastDelegateGenericCallback* func;
			void* userdata;
			void Call(Args... args) override { func(userdata); }
		};
		auto* e = new GFPEntry;
		e->func = func;
		e->userdata = userdata;
		_AddEntry(e);
		return e;
	}

	void Remove(Entry* e)
	{
		assert(e->_owner == this);
		if (e->_prev)
			e->_prev->_next = e->_next;
		else
			_first = e->_next;
		if (e->_next)
			e->_next->_prev = e->_prev;
		else
			_last = e->_prev;
		e->_owner = (MulticastDelegate*)0xDE1E7ED;
		delete e;
	}

	void Call(Args... args) const
	{
		for (Entry* e = _first; e; e = e->_next)
			e->Call(args...);
	}
};

} // ui
