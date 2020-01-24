
#pragma once
#include <type_traits>

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
