
#pragma once

#include <type_traits>


namespace ui {

struct AtomicBool
{
	AtomicBool(bool v);
	bool Load() const;
	void Store(bool v);

	operator bool() { return Load(); }

private:
	int32_t _mem;
};

struct AtomicInt32
{
	AtomicInt32(int32_t v);
	int32_t Load() const;
	void Store(int32_t v);
	AtomicInt32& operator ++ ();
	AtomicInt32& operator -- ();

	operator int32_t () const { return Load(); }
	AtomicInt32& operator = (const AtomicInt32& o)
	{
		Store(o);
		return *this;
	}
	AtomicInt32 operator ++ (int)
	{
		auto copy = *this;
		++*this;
		return copy;
	}
	AtomicInt32 operator -- (int)
	{
		auto copy = *this;
		--*this;
		return copy;
	}

private:
	int32_t _mem;
};

struct EventQueue
{
	struct Entry
	{
		virtual ~Entry() {}
		virtual void Run() = 0;
	};

	EventQueue();
	~EventQueue();
	void _AddToQueue(Entry* e, bool clear);
	void Clear();
	bool RunOne();
	void RunAllCurrent();

	template <class F> void Push(F&& f, bool clear = false)
	{
		static_assert(std::is_rvalue_reference<F&&>::value, "not an rvalue reference");
		struct Func : Entry
		{
			Func(F&& _f) : f(std::move(_f)) {}
			void Run() override
			{
				f();
			}
			F f;
		};
		_AddToQueue(new Func(std::move(f)), clear);
	}

	struct EventQueueImpl* _impl;
};

struct WorkerQueue
{
	struct Entry
	{
		virtual ~Entry() {}
		virtual void Run() = 0;
	};

	WorkerQueue();
	~WorkerQueue();
	void _AddToQueue(Entry* e, bool clear);
	void Clear();

	bool HasItems();
	bool IsQuitting();

	template <class F> void Push(F&& f, bool clear = false)
	{
		static_assert(std::is_rvalue_reference<F&&>::value, "not an rvalue reference");
		struct Func : Entry
		{
			Func(F&& _f) : f(std::move(_f)) {}
			void Run() override
			{
				f();
			}
			F f;
		};
		_AddToQueue(new Func(std::move(f)), clear);
	}

	struct WorkerQueueImpl* _impl;
};

} // ui
