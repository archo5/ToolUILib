
#pragma once

#include "Threading.h"


namespace ui {

struct IRefCounted
{
	virtual void AddRef() = 0;
	virtual void Release() = 0;
};

class RefCountedST : IRefCounted
{
	int32_t _refCount = 0;

public:
	virtual ~RefCountedST() {}

	void AddRef() { _refCount++; }
	void Release()
	{
		if (--_refCount == 0)
			delete this;
	}
};

class RefCountedMT : IRefCounted
{
	AtomicInt32 _refCount = 0;

public:
	virtual ~RefCountedMT() {}

	void AddRef() { _refCount++; }
	void Release()
	{
		if (--_refCount == 0)
			delete this;
	}
};

template <class T>
class RCHandle
{
	template<typename> friend class RCHandle;

	T* _ptr;

public:
	UI_FORCEINLINE RCHandle() : _ptr(nullptr) {}
	UI_FORCEINLINE RCHandle(std::nullptr_t) : _ptr(nullptr) {}
	UI_FORCEINLINE RCHandle(T* ptr) : _ptr(ptr)
	{
		if (ptr)
			ptr->AddRef();
	}
	UI_FORCEINLINE RCHandle(const RCHandle& o) : _ptr(o._ptr)
	{
		if (_ptr)
			_ptr->AddRef();
	}
	UI_FORCEINLINE RCHandle(RCHandle&& o) : _ptr(o._ptr)
	{
		o._ptr = nullptr;
	}
	template <class OT>
	UI_FORCEINLINE RCHandle(const RCHandle<OT>& o) : _ptr(o._ptr)
	{
		if (_ptr)
			_ptr->AddRef();
	}
	template <class OT>
	UI_FORCEINLINE RCHandle(RCHandle<OT>&& o) : _ptr(o._ptr)
	{
		o._ptr = nullptr;
	}
	~RCHandle()
	{
		if (_ptr)
		{
			_ptr->Release();
			_ptr = nullptr;
		}
	}
	RCHandle& operator = (T* ptr)
	{
		if (_ptr != ptr)
		{
			auto* pptr = _ptr;
			_ptr = ptr;
			if (ptr)
				ptr->AddRef();
			if (pptr)
				pptr->Release();
		}
		return *this;
	}
	RCHandle& operator = (const RCHandle& o)
	{
		if (_ptr != o._ptr)
		{
			auto* pptr = _ptr;
			_ptr = o._ptr;
			if (_ptr)
				_ptr->AddRef();
			if (pptr)
				pptr->Release();
		}
		return *this;
	}
	RCHandle& operator = (RCHandle&& o)
	{
		if (this != &o)
		{
			auto* pptr = _ptr;
			_ptr = o._ptr;
			o._ptr = nullptr;
			if (pptr)
				pptr->Release();
		}
		return *this;
	}
	template <class OT>
	RCHandle& operator = (const RCHandle<OT>& o)
	{
		if (_ptr != o._ptr)
		{
			auto* pptr = _ptr;
			_ptr = o._ptr;
			if (_ptr)
				_ptr->AddRef();
			if (pptr)
				pptr->Release();
		}
		return *this;
	}
	template <class OT>
	RCHandle& operator = (RCHandle<OT>&& o)
	{
		if (this != &o)
		{
			auto* pptr = _ptr;
			_ptr = o._ptr;
			o._ptr = nullptr;
			if (pptr)
				pptr->Release();
		}
		return *this;
	}

	void Release()
	{
		if (_ptr)
		{
			_ptr->Release();
			_ptr = nullptr;
		}
	}

	//operator void* () const { return _ptr; }
	UI_FORCEINLINE T& operator * () const { return *_ptr; }
	UI_FORCEINLINE T* operator -> () const { return _ptr; }
	UI_FORCEINLINE operator T* () const { return _ptr; }
	UI_FORCEINLINE T* get_ptr() const { return _ptr; }
#if 0
	bool operator == (const T* o) const { return _ptr == o; }
	bool operator != (const T* o) const { return _ptr != o; }
	bool operator == (const RCHandle& o) const { return _ptr == o._ptr; }
	bool operator != (const RCHandle& o) const { return _ptr != o._ptr; }
#endif
};

template <class T> UI_FORCEINLINE RCHandle<T> AsRCHandle(T* ptr) { return ptr; }

} // ui
