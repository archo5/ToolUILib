
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <assert.h>

#include "Threading.h"


static_assert(sizeof(std::atomic<bool>) <= sizeof(int32_t), "unexpected atomic bool size");

AtomicBool::AtomicBool(bool v)
{
	reinterpret_cast<std::atomic_bool*>(&_mem)->store(v);
}

bool AtomicBool::Load() const
{
	return reinterpret_cast<const std::atomic_bool*>(&_mem)->load();
}

void AtomicBool::Store(bool v)
{
	reinterpret_cast<std::atomic_bool*>(&_mem)->store(v);
}


static_assert(sizeof(std::atomic<int32_t>) == sizeof(int32_t), "unexpected atomic int32 size");

AtomicInt32::AtomicInt32(int32_t v)
{
	reinterpret_cast<std::atomic<int32_t>*>(&_mem)->store(v);
}

int32_t AtomicInt32::Load() const
{
	return reinterpret_cast<const std::atomic<int32_t>*>(&_mem)->load();
}

void AtomicInt32::Store(int32_t v)
{
	reinterpret_cast<std::atomic<int32_t>*>(&_mem)->store(v);
}

AtomicInt32& AtomicInt32::operator ++ ()
{
	(*reinterpret_cast<std::atomic<int32_t>*>(&_mem))++;
	return *this;
}

AtomicInt32& AtomicInt32::operator -- ()
{
	(*reinterpret_cast<std::atomic<int32_t>*>(&_mem))--;
	return *this;
}


struct EventQueueImpl
{
	std::queue<EventQueue::Entry*> q;
	std::mutex m;
};

EventQueue::EventQueue()
{
	_impl = new EventQueueImpl;
}

EventQueue::~EventQueue()
{
	delete _impl;
}

void EventQueue::_AddToQueue(Entry* e, bool clear)
{
	std::lock_guard<std::mutex> g(_impl->m);
	if (clear)
	{
		while (!_impl->q.empty())
		{
			delete _impl->q.front();
			_impl->q.pop();
		}
	}
	_impl->q.push(e);
}

void EventQueue::Clear()
{
	std::lock_guard<std::mutex> g(_impl->m);
	while (!_impl->q.empty())
	{
		delete _impl->q.front();
		_impl->q.pop();
	}
}

bool EventQueue::RunOne()
{
	Entry* e = nullptr;
	{
		std::lock_guard<std::mutex> g(_impl->m);
		if (_impl->q.empty())
			return false;
		e = _impl->q.front();
		_impl->q.pop();
	}
	e->Run();
	delete e;
	return true;
}

void EventQueue::RunAllCurrent()
{
	_impl->m.lock();
	if (_impl->q.empty())
	{
		_impl->m.unlock();
		return;
	}

	auto* end = _impl->q.back();
	while (!_impl->q.empty())
	{
		auto* e = _impl->q.front();
		_impl->q.pop();
		_impl->m.unlock();

		e->Run();
		delete e;

		if (e == end)
			return;

		_impl->m.lock();
	}
	_impl->m.unlock();
}


struct WorkerQueueImpl
{
	std::queue<WorkerQueue::Entry*> q;
	std::mutex m;
	std::condition_variable cv;
	std::thread t;
	bool quit = false;
};

static void WorkerQueueProc(WorkerQueueImpl* wqi)
{
	std::unique_lock<std::mutex> ulk(wqi->m);
	for (;;)
	{
		while (!(wqi->quit || !wqi->q.empty()))
			wqi->cv.wait(ulk);
		if (wqi->quit && wqi->q.empty())
			return;
		auto* e = wqi->q.front();
		wqi->q.pop();
		ulk.unlock();
		e->Run();
		delete e;
		ulk.lock();
	}
}

WorkerQueue::WorkerQueue()
{
	_impl = new WorkerQueueImpl;
	_impl->t = std::thread(WorkerQueueProc, _impl);
}

WorkerQueue::~WorkerQueue()
{
	{
		std::lock_guard<std::mutex> g(_impl->m);
		_impl->quit = true;
	}
	_impl->cv.notify_one();
	_impl->t.join();
	delete _impl;
}

void WorkerQueue::_AddToQueue(Entry* e, bool clear)
{
	{
		std::lock_guard<std::mutex> g(_impl->m);
		assert(!_impl->quit);
		if (clear)
		{
			while (!_impl->q.empty())
			{
				delete _impl->q.front();
				_impl->q.pop();
			}
		}
		_impl->q.push(e);
	}
	_impl->cv.notify_one();
}

void WorkerQueue::Clear()
{
	std::lock_guard<std::mutex> g(_impl->m);
	while (!_impl->q.empty())
	{
		delete _impl->q.front();
		_impl->q.pop();
	}
}

bool WorkerQueue::HasItems()
{
	std::lock_guard<std::mutex> g(_impl->m);
	return !_impl->q.empty();
}

bool WorkerQueue::IsQuitting()
{
	return _impl->quit;
}
