
#pragma once
#include "Model/Objects.h"


namespace ui {


struct ListLayoutSlotBase
{
	UIObject* _obj = nullptr;
};


template <class SlotT>
struct ListLayoutElementBase : UIObject
{
	using Slot = SlotT;

	Array<Slot> _slots;

	// size cache
	uint32_t _cacheFrameWidth = {};
	uint32_t _cacheFrameHeight = {};
	EstSizeRange _cacheValueWidth;
	EstSizeRange _cacheValueHeight;

	virtual Slot _CopyAndResetSlotTemplate() { return {}; }

	void OnReset() override
	{
		UIObject::OnReset();

		_slots.Clear();
	}

	void SlotIterator_Init(UIObjectIteratorData& data) override
	{
		data.data0 = 0;
	}

	UIObject* SlotIterator_GetNext(UIObjectIteratorData& data) override
	{
		if (data.data0 >= _slots.Size())
			return nullptr;
		return _slots[data.data0++]._obj;
	}

	void RemoveChildImpl(UIObject* ch) override
	{
		for (size_t i = 0; i < _slots.Size(); i++)
		{
			if (_slots[i]._obj == ch)
			{
				_slots.RemoveAt(i);
				break;
			}
		}
	}

	void DetachChildren(bool recursive) override
	{
		for (size_t i = 0; i < _slots.Size(); i++)
		{
			auto* ch = _slots[i]._obj;

			if (recursive)
				ch->DetachChildren(true);

			// if ch->system != 0 then system != 0 but the latter should be a more predictable branch
			if (system)
				ch->_DetachFromTree();

			ch->parent = nullptr;
		}
		_slots.Clear();
	}

	void AppendChild(UIObject* obj) override
	{
		obj->DetachParent();

		obj->parent = this;
		Slot slot = _CopyAndResetSlotTemplate();
		slot._obj = obj;
		_slots.Append(slot);

		if (system)
			obj->_AttachToFrameContents(system);
	}

	void OnPaint(const UIPaintContext& ctx) override
	{
		for (auto& slot : _slots)
			slot._obj->Paint(ctx);
	}

	UIObject* FindObjectAtPoint(Point2f pos) override
	{
		for (size_t i = _slots.Size(); i > 0; )
		{
			i--;
			if (_slots[i]._obj->Contains(pos))
				if (auto* o = _slots[i]._obj->FindObjectAtPoint(pos))
					return o;
		}
		return nullptr;
	}

	void _AttachToFrameContents(FrameContents* owner) override
	{
		UIObject::_AttachToFrameContents(owner);

		for (auto& slot : _slots)
			slot._obj->_AttachToFrameContents(owner);
	}

	void _DetachFromFrameContents() override
	{
		for (auto& slot : _slots)
			slot._obj->_DetachFromFrameContents();

		UIObject::_DetachFromFrameContents();
	}

	void _DetachFromTree() override
	{
		if (!(flags & UIObject_IsInTree))
			return;

		for (auto& slot : _slots)
			slot._obj->_DetachFromTree();

		UIObject::_DetachFromTree();
	}
};


} // ui
