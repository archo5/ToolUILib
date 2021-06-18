
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
	RCHandle() : _ptr(nullptr) {}
	RCHandle(std::nullptr_t) : _ptr(nullptr) {}
	RCHandle(T* ptr) : _ptr(ptr)
	{
		if (ptr)
			ptr->AddRef();
	}
	RCHandle(const RCHandle& o) : _ptr(o._ptr)
	{
		if (_ptr)
			_ptr->AddRef();
	}
	RCHandle(RCHandle&& o) : _ptr(o._ptr)
	{
		o._ptr = nullptr;
	}
	template <class OT>
	RCHandle(const RCHandle<OT>& o) : _ptr(o._ptr)
	{
		if (_ptr)
			_ptr->AddRef();
	}
	template <class OT>
	RCHandle(RCHandle<OT>&& o) : _ptr(o._ptr)
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
			if (_ptr)
				_ptr->Release();
			_ptr = ptr;
			if (ptr)
				ptr->AddRef();
		}
		return *this;
	}
	RCHandle& operator = (const RCHandle& o)
	{
		if (_ptr != o._ptr)
		{
			if (_ptr)
				_ptr->Release();
			_ptr = o._ptr;
			if (_ptr)
				_ptr->AddRef();
		}
		return *this;
	}
	RCHandle& operator = (RCHandle&& o)
	{
		if (this != &o)
		{
			if (_ptr)
				_ptr->Release();
			_ptr = o._ptr;
			o._ptr = nullptr;
		}
		return *this;
	}
	template <class OT>
	RCHandle& operator = (const RCHandle<OT>& o)
	{
		if (_ptr != o._ptr)
		{
			if (_ptr)
				_ptr->Release();
			_ptr = o._ptr;
			if (_ptr)
				_ptr->AddRef();
		}
		return *this;
	}
	template <class OT>
	RCHandle& operator = (RCHandle<OT>&& o)
	{
		if (this != &o)
		{
			if (_ptr)
				_ptr->Release();
			_ptr = o._ptr;
			o._ptr = nullptr;
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
	T& operator * () const { return *_ptr; }
	T* operator -> () const { return _ptr; }
	operator T* () const { return _ptr; }
	T* get_ptr() const { return _ptr; }
#if 0
	bool operator == (const T* o) const { return _ptr == o; }
	bool operator != (const T* o) const { return _ptr != o; }
	bool operator == (const RCHandle& o) const { return _ptr == o._ptr; }
	bool operator != (const RCHandle& o) const { return _ptr != o._ptr; }
#endif
};

template <class T> inline RCHandle<T> AsRCHandle(T* ptr) { return ptr; }

} // ui
