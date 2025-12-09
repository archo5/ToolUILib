
#pragma once
#include "EditCommon.h"

#include "../Model/Objects.h"
#include "../Model/Controls.h"
#include "../Model/System.h"


namespace ui {

struct ISequence
{
	virtual size_t GetSizeLimit() = 0;
	virtual size_t GetCurrentSize() = 0;

	using IterationFunc = std::function<bool(size_t idx, void* ptr)>;
	virtual void IterateElements(size_t from, IterationFunc&& fn) = 0;

	virtual void Remove(size_t off) {}
	// insert a copy from [off] at [off + 1]
	virtual void Duplicate(size_t off) {}
	virtual void MoveElementTo(size_t off, size_t to) {}

	// drag & drop between different sequences
	virtual void* GetContainerPtr() const { return nullptr; }
	bool AreContainersEqual(ISequence* o) const
	{
		void* cpc = GetContainerPtr();
		if (!cpc)
			return false;
		void* cpo = o->GetContainerPtr();
		if (!cpo)
			return false;
		return cpc == cpo;
	}
	virtual const type_info* GetElementType() const { return nullptr; }
	bool AreElementTypesEqual(ISequence* o) const
	{
		const type_info* etc = GetElementType();
		if (!etc)
			return false;
		const type_info* eto = o->GetElementType();
		if (!eto)
			return false;
		return etc == eto;
	}
	// - assume same element type
	// - assume the containers are NOT equal
	virtual bool MoveElementFromOtherSeq(size_t dstoff, ISequence* src, size_t srcoff) { return false; }
};


template <class Cont>
struct ArraySequence : ISequence
{
	using ContainerType = std::remove_reference_t<Cont>;
	ContainerType& cont;
	size_t sizeLimit = SIZE_MAX;

	ArraySequence(ContainerType& c) : cont(c) {}

	size_t GetSizeLimit() override
	{
		return sizeLimit;
	}
	size_t GetCurrentSize() override
	{
		return cont.Size();
	}

	void IterateElements(size_t from, IterationFunc&& fn) override
	{
		for (size_t i = from; i < cont.Size(); i++)
			if (!fn(i, &cont[i]))
				break;
	}

	void Remove(size_t off) override
	{
		cont.RemoveAt(off);
	}
	void Duplicate(size_t off) override
	{
		auto val = cont[off];
		cont.InsertAt(off + 1, val);
	}
	void MoveElementTo(size_t off, size_t to) override
	{
		auto imin = min(off, to);
		auto imax = max(off, to);
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

	void* GetContainerPtr() const override
	{
		return &cont;
	}
	const type_info* GetElementType() const override
	{
		return &typeid(ContainerType::ValueType);
	}
	bool MoveElementFromOtherSeq(size_t insoff, ISequence* src, size_t srcoff) override
	{
		bool found = false;
		src->IterateElements(srcoff, [this, insoff, &found](size_t idx, void* ptr) -> bool
		{
			cont.InsertAt(insoff, std::move(*static_cast<typename ContainerType::ValueType*>(ptr)));
			found = true;
			return false;
		});
		if (found)
			src->Remove(srcoff);
		return found;
	}
};


template <class Cont>
struct StdSequence : ISequence
{
	using ContainerType = std::remove_reference_t<Cont>;
	ContainerType& cont;
	size_t sizeLimit = SIZE_MAX;

	StdSequence(ContainerType& c) : cont(c) {}

	size_t GetSizeLimit() override
	{
		return sizeLimit;
	}
	size_t GetCurrentSize() override
	{
		return cont.size();
	}

	void IterateElements(size_t from, IterationFunc&& fn) override
	{
		auto it = cont.begin();
		std::advance(it, from);
		for (size_t idx = from; it != cont.end(); it++, idx++)
			if (!fn(idx, &*it))
				break;
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
		auto imin = min(off, to);
		auto imax = max(off, to);
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

	void* GetContainerPtr() const override
	{
		return &cont;
	}
	const type_info* GetElementType() const override
	{
		return &typeid(ContainerType::value_type);
	}
	bool MoveElementFromOtherSeq(size_t insoff, ISequence* src, size_t srcoff) override
	{
		bool found = false;
		src->IterateElements(srcoff, [this, insoff, &found](size_t idx, void* ptr) -> bool
		{
			auto it = cont.begin();
			std::advance(it, insoff);
			cont.insert(it, std::move(*static_cast<typename ContainerType::value_type*>(ptr)));
			found = true;
			return false;
		});
		if (found)
			src->Remove(srcoff);
		return found;
	}
};


template <class Type, class Size>
struct BufferSequence : ISequence
{
	Type* data;
	Size& size;
	size_t limit;

	BufferSequence(Type* d, Size& sz, size_t lim) : data(d), size(sz), limit(lim) {}
	template <size_t N>
	BufferSequence(Type(&d)[N], Size& sz) : data(d), size(sz), limit(N) {}

	size_t GetSizeLimit() override
	{
		return limit;
	}
	size_t GetCurrentSize() override
	{
		return size;
	}

	void IterateElements(size_t from, IterationFunc&& fn) override
	{
		for (size_t i = from; i < size; i++)
			if (!fn(i, &data[i]))
				break;
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

	void* GetContainerPtr() const override
	{
		return data;
	}
	const type_info* GetElementType() const override
	{
		return &typeid(Type);
	}
	bool MoveElementFromOtherSeq(size_t insoff, ISequence* src, size_t srcoff) override
	{
		if (size == limit)
			return false;
		bool found = false;
		src->IterateElements(srcoff, [this, insoff, &found](size_t idx, void* ptr) -> bool
		{
			if (insoff < size)
				std::move_backward(data + insoff, data + size, data + size + 1);
			data[insoff] = std::move(*(Type*)ptr);
			size++;
			found = true;
			return false;
		});
		if (found)
			src->Remove(srcoff);
		return found;
	}
};


struct SequenceEditor;
struct SequenceDragData : DragDropData
{
	static constexpr const char* NAME = "SequenceDragData";

	SequenceDragData(SequenceEditor* s, float w, size_t f) :
		DragDropData(NAME),
		scope(s),
		width(w),
		at(f)
	{}
	void Build() override;

	SequenceEditor* scope;
	float width;
	size_t at;
};

struct SequenceItemElement : Selectable
{
	void OnReset() override;
	void OnEvent(Event& e) override;
	virtual void ContextMenu();

	void Init(SequenceEditor* se, size_t n);

	SequenceEditor* seqEd = nullptr;
	size_t num = 0;
};

enum SequenceElementDragSupport : u8
{
	None,
	SameSequenceEditor,
	SameContainer,
	SameType,
};

struct ISequenceElementParentCheck
{
	virtual bool SEPC_IsThisContainedBy(void* itemptr) = 0;
};

struct SequenceEditor : Buildable
{
	void Build() override;
	void OnEvent(Event& e) override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnReset() override;

	virtual void OnBuildItem(size_t idx, void* ptr);
	virtual void OnBuildDeleteButton();

	ISequence* GetSequence() const { return _sequence; }
	SequenceEditor& SetSequence(ISequence* s);
	ISelectionStorage* GetSelectionStorage() const { return _selStorage; }
	SequenceEditor& SetSelectionStorage(ISelectionStorage* src);
	SequenceEditor& SetSelectionMode(SelectionMode mode);
	IListContextMenuSource* GetContextMenuSource() const { return _ctxMenuSrc; }
	SequenceEditor& SetContextMenuSource(IListContextMenuSource* src);

	void _OnEdit(UIObject* who);
	void _OnDelete(SequenceItemElement* sie);

	std::function<void(SequenceEditor* se, size_t idx, void* ptr)> itemUICallback;
	std::function<EditorActionResponse(size_t idx)> onBeforeRemoveElement;
	EditorItemContentsLayoutPreset itemLayoutPreset = EditorItemContentsLayoutPreset::StackExpandLTRWithDeleteButton;
	bool buildFrame = true;
	bool allowDelete = true;
	bool allowDuplicate = true;
	SequenceElementDragSupport dragSupport = SequenceElementDragSupport::SameSequenceEditor;
	SequenceEditor& SetDragSupport(SequenceElementDragSupport ds) { dragSupport = ds; return *this; }
	ISequenceElementParentCheck* sameTypeDragParentCheck = nullptr;
	SequenceEditor& SetSameTypeDragParentCheck(ISequenceElementParentCheck* sepc) { sameTypeDragParentCheck = sepc; return *this; }

	ISequence* _sequence = nullptr;
	ISelectionStorage* _selStorage = nullptr;
	IListContextMenuSource* _ctxMenuSrc = nullptr;

	size_t _dragTargetPos = SIZE_MAX;
	UIRect _dragTargetLine = {};
	SelectionImplementation _selImpl;
};

} // ui
