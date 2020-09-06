
#pragma once

#include "Objects.h"
#include "Controls.h"
#include "System.h"


namespace ui {

struct ISequenceIterator
{
	virtual ~ISequenceIterator() {}
};

struct ISequence
{
	virtual size_t GetSizeLimit() = 0;
	virtual size_t GetCurrentSize() = 0;
	virtual ISequenceIterator* GetIterator(size_t off = 0) = 0;
	virtual bool AtEnd(ISequenceIterator*) = 0;
	virtual void Advance(ISequenceIterator*) = 0;
	virtual void* GetValuePtr(ISequenceIterator*) = 0;
	virtual size_t GetOffset(ISequenceIterator*) = 0;

	template <class T> T& GetValue(ISequenceIterator* it) { return *static_cast<T*>(GetValuePtr(it)); }

	virtual void Remove(size_t off) {}
	// insert a copy from [off] at [off + 1]
	virtual void Duplicate(size_t off) {}
	virtual void MoveElementTo(size_t off, size_t to) {}
};


template <class Cont>
struct StdSequence : ISequence
{
	using std_iterator = typename Cont::iterator;

	struct Iterator : ISequenceIterator
	{
		Iterator(std_iterator&& i, size_t o) : it(i), off(o) {}

		std_iterator it;
		size_t off;
	};

	size_t GetSizeLimit() override
	{
		return sizeLimit;
	}
	size_t GetCurrentSize() override
	{
		return cont.size();
	}
	ISequenceIterator* GetIterator(size_t off = 0) override
	{
		auto it = cont.begin();
		std::advance(it, off);
		return new Iterator(std::move(it), off);
	}
	bool AtEnd(ISequenceIterator* it) override
	{
		return static_cast<Iterator*>(it)->it == cont.end();
	}
	void Advance(ISequenceIterator* it) override
	{
		auto* iit = static_cast<Iterator*>(it);
		++iit->it;
		++iit->off;
	}
	void* GetValuePtr(ISequenceIterator* it) override
	{
		return &*static_cast<Iterator*>(it)->it;
	}
	size_t GetOffset(ISequenceIterator* it) override
	{
		return static_cast<Iterator*>(it)->off;
	}

	void Remove(size_t off) override
	{
		auto it = cont.begin();
		std::advance(it, off);
		cont.erase(it);
	}
	void Duplicate(size_t off) override
	{
		auto it = cont.begin();
		std::advance(it, off);
		auto val = *it;
		std::advance(it, 1);
		cont.insert(it, val);
	}
	void MoveElementTo(size_t off, size_t to) override
	{
		auto imin = std::min(off, to);
		auto imax = std::max(off, to);
		auto minit = cont.begin();
		std::advance(minit, imin);
		auto maxit = minit;
		std::advance(maxit, imax - imin);
		auto itoff = off == imin ? minit : maxit;
		auto itto = to == imin ? minit : maxit;
		for (auto it = itoff; it != itto; )
		{
			auto nit = it;
			if (to > off)
				++nit;
			else
				--nit;
			std::swap(*it, *nit);
			it = nit;
		}
	}

	StdSequence(Cont& c) : cont(c) {}

	Cont& cont;
	size_t sizeLimit = SIZE_MAX;
};


template <class Type, class Size>
struct BufferSequence : ISequence
{
	struct Iterator : ISequenceIterator
	{
		Iterator(size_t o) : off(o) {}

		size_t off;
	};

	size_t GetSizeLimit() override
	{
		return limit;
	}
	size_t GetCurrentSize() override
	{
		return size;
	}
	ISequenceIterator* GetIterator(size_t off = 0) override
	{
		return new Iterator(off);
	}
	bool AtEnd(ISequenceIterator* it) override
	{
		return static_cast<Iterator*>(it)->off == size;
	}
	void Advance(ISequenceIterator* it) override
	{
		static_cast<Iterator*>(it)->off++;
	}
	void* GetValuePtr(ISequenceIterator* it) override
	{
		return &data[static_cast<Iterator*>(it)->off];
	}
	size_t GetOffset(ISequenceIterator* it) override
	{
		return static_cast<Iterator*>(it)->off;
	}

	void Remove(size_t off) override
	{
		assert(size > 0);
		if (off + 1 < size)
			std::move(data + off + 1, data + size, data + off);
		else
			data[off] = {};
		size--;
	}
	void Duplicate(size_t off) override
	{
		assert(size < limit);
		if (off + 1 < size)
			std::move_backward(data + off + 1, data + size, data + size + 1);
		data[off + 1] = data[off];
		size++;
	}
	void MoveElementTo(size_t off, size_t to) override
	{
		while (off < to)
		{
			std::swap(data[off], data[off + 1]);
			off++;
		}
		while (off > to)
		{
			std::swap(data[off], data[off - 1]);
			off--;
		}
	}

	BufferSequence(Type* d, Size& sz, size_t lim) : data(d), size(sz), limit(lim) {}
	template <size_t N>
	BufferSequence(Type(&d)[N], Size& sz) : data(d), size(sz), limit(N) {}

	Type* data;
	Size& size;
	size_t limit;
};


struct SequenceEditor;
struct SequenceDragData : DragDropData
{
	static constexpr const char* Name = "SequenceDragData";

	SequenceDragData(SequenceEditor* s, size_t f) : ui::DragDropData(Name), scope(s), at(f) {}

	SequenceEditor* scope;
	size_t at;
};

struct SequenceItemElement : Selectable
{
	SequenceItemElement();
	void OnEvent(UIEvent& e) override;
	virtual void ContextMenu();

	void Init(SequenceEditor* se, size_t n);

	SequenceEditor* seqEd = nullptr;
	size_t num = 0;
};

struct SequenceEditor : Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override;

	virtual void OnBuildItem(UIContainer* ctx, ISequenceIterator* it);
	virtual void OnBuildDeleteButton(UIContainer* ctx, ISequenceIterator* it);

	ISequence* GetSequence() const { return _sequence; }
	SequenceEditor& SetSequence(ISequence* s);

	std::function<void(UIContainer* ctx, SequenceEditor* se, ISequenceIterator* it)> itemUICallback;

	ISequence* _sequence;
};

} // ui
