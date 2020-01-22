
#pragma once
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <typeinfo>
#include <new>

#include "../Core/Math.h"
#include "../Core/Serialization.h"
#include "../Core/String.h"

#include "../Render/Render.h"

#include "Events.h"
#include "Layout.h"


struct UIEvent;
class UIObject; // any item
class UIElement; // physical item
class UIContainer;
class UIEventSystem;

namespace prs {
class PropertyBlock;
enum class Unit;
}

namespace style {
struct Block;
}

namespace ui {
class NativeWindowBase;
class FrameContents;
class Node; // logical item

using EventFunc = std::function<void(UIEvent& e)>;
}


enum UIObjectFlags
{
	UIObject_IsInRenderStack = 1 << 0,
	UIObject_IsInLayoutStack = 1 << 1,
	UIObject_IsHovered = 1 << 2,
	_UIObject_IsClicked_First = 1 << 3,
	UIObject_IsClickedL = 1 << 3,
	UIObject_IsClickedR = 1 << 4,
	UIObject_IsClickedM = 1 << 5,
	UIObject_IsClickedX1 = 1 << 6,
	UIObject_IsClickedX2 = 1 << 7,
	UIObject_IsClickedAnyMask = (UIObject_IsClickedL | UIObject_IsClickedR
		| UIObject_IsClickedM | UIObject_IsClickedX1 | UIObject_IsClickedX2),
	UIObject_IsDisabled = 1 << 8,
	UIObject_IsHidden = 1 << 9,
	UIObject_DragHovered = 1 << 10, // TODO reorder
};

enum class Direction
{
	Horizontal,
	Vertical,
};

class UIObject
{
public:
	UIObject();
	virtual ~UIObject();
	virtual void Reset() {}
	virtual void OnInit() {}
	virtual void OnDestroy() {}
	virtual void OnCompleteStructure() {}
	virtual void OnEvent(UIEvent& e) {}
	virtual void OnPaint();
	void Paint();
	void PaintChildren()
	{
		for (auto* ch = firstChild; ch; ch = ch->next)
			ch->Paint();
	}
	virtual void GetSize(style::Coord& outWidth, style::Coord& outHeight) {}
	virtual float CalcEstimatedWidth(const Size<float>& containerSize);
	virtual float CalcEstimatedHeight(const Size<float>& containerSize);
	Range<float> GetEstimatedWidth(const Size<float>& containerSize);
	Range<float> GetEstimatedHeight(const Size<float>& containerSize);
	virtual Range<float> GetFullEstimatedWidth(const Size<float>& containerSize);
	virtual Range<float> GetFullEstimatedHeight(const Size<float>& containerSize);
	void PerformLayout(const UIRect& rect, const Size<float>& containerSize);
	virtual void OnLayout(const UIRect& rect, const Size<float>& containerSize);
	virtual bool Contains(float x, float y) const
	{
		return GetBorderRect().Contains(x, y);
	}

	bool _CanPaint() const { return !(flags & UIObject_IsHidden); }
	bool _NeedsLayout() const { return !(flags & UIObject_IsHidden); }

	bool IsChildOf(UIObject* obj) const;
	bool IsChildOrSame(UIObject* obj) const;
	int CountChildrenImmediate() const;
	int CountChildrenRecursive() const;
	int GetDepth() const
	{
		int out = 0;
		for (auto* p = this; p; p = p->parent)
			out++;
		return out;
	}
	template <class T> T* FindParentOfType()
	{
		for (auto* p = parent; p; p = p->parent)
			if (auto* t = dynamic_cast<T*>(p))
				return t;
		return nullptr;
	}
	void RerenderNode();

	bool IsHovered() const;
	bool IsClicked(int button = 0) const;
	bool IsFocused() const;

	bool IsVisible() const;
	void SetVisible(bool v);

	bool IsInputDisabled() const;
	void SetInputDisabled(bool v);

	style::Accessor GetStyle();
	void SetStyle(style::Block* style);
	float ResolveUnits(style::Coord coord, float ref);
	UIRect GetMarginRect(style::Block* style, float ref);
	UIRect GetPaddingRect(style::Block* style, float ref);

	UIRect GetContentRect() const { return finalRectC; }
	UIRect GetPaddingRect() const { return finalRectCP; }
	UIRect GetBorderRect() const { return finalRectCPB; }

	ui::NativeWindowBase* GetNativeWindow() const;

	void dump() { printf("    [=%p ]=%p ^=%p <=%p >=%p\n", firstChild, lastChild, parent, prev, next); fflush(stdout); }

	uint32_t flags = 0;
	UIObject* parent = nullptr;
	UIObject* firstChild = nullptr;
	UIObject* lastChild = nullptr;
	UIObject* prev = nullptr;
	UIObject* next = nullptr;

	ui::FrameContents* system = nullptr;
	style::BlockRef styleProps;

	// final layout rectangles: C=content, P=padding, B=border
	UIRect finalRectC;
	UIRect finalRectCP;
	UIRect finalRectCPB;
};

class UIElement : public UIObject
{
public:
	typedef char IsElement[1];
};

namespace ui {

class TextElement : public UIElement
{
public:
	TextElement()
	{
		GetStyle().SetLayout(style::layouts::InlineBlock());
	}
	float CalcEstimatedWidth(const Size<float>& containerSize) override
	{
		return ceilf(GetTextWidth(text.c_str()));
	}
	float CalcEstimatedHeight(const Size<float>& containerSize) override
	{
		return GetFontHeight();
	}
	void OnLayout(const UIRect& rect, const Size<float>& containerSize) override
	{
		finalRectC = finalRectCP = finalRectCPB = rect;
		//finalRect.x1 = finalRect.x0 + GetTextWidth(text)
		//finalRect.y1 = finalRect.y0 + GetFontHeight();
	}
	void OnPaint() override
	{
		auto r = GetContentRect();
		float w = r.x1 - r.x0;
		DrawTextLine(r.x0, r.y1 - (r.y1 - r.y0 - GetFontHeight()) / 2, text.c_str(), 1, 1, 1);
		PaintChildren();
	}
	void SetText(StringView t)
	{
		text.assign(t.data(), t.size());
	}

	std::string text;
};

class BoxElement : public UIElement
{
};

struct EventHandlerEntry;
struct Subscription;
struct DataCategoryTag {};

constexpr auto ANY_ITEM = uintptr_t(-1);

void Notify(DataCategoryTag* tag, uintptr_t at = ANY_ITEM);
inline void Notify(DataCategoryTag* tag, const void* ptr)
{
	Notify(tag, reinterpret_cast<uintptr_t>(ptr));
}

class Node : public UIObject
{
public:
	~Node();
	typedef char IsNode[2];

	void OnEvent(UIEvent& e) override;
	virtual void Render(UIContainer* ctx) = 0;
	void Rerender();

	bool Subscribe(DataCategoryTag* tag, uintptr_t at = ANY_ITEM);
	bool Unsubscribe(DataCategoryTag* tag, uintptr_t at = ANY_ITEM);
	bool Subscribe(DataCategoryTag* tag, const void* ptr)
	{
		return Subscribe(tag, reinterpret_cast<uintptr_t>(ptr));
	}
	bool Unsubscribe(DataCategoryTag* tag, const void* ptr)
	{
		return Unsubscribe(tag, reinterpret_cast<uintptr_t>(ptr));
	}

	EventFunc& HandleEvent(UIObject* target = nullptr, UIEventType type = UIEventType::Any);
	void ClearEventHandlers();

private:
	friend struct Subscription;
	Subscription* _firstSub = nullptr;
	Subscription* _lastSub = nullptr;
	friend struct EventHandlerEntry;
	EventHandlerEntry* _firstEH = nullptr;
};

} // ui
