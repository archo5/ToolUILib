
#pragma once
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <typeinfo>
#include <new>

#include "../Core/Math.h"

#include "../Render/Theme.h"

#include "Events.h"
#include "Layout.h"


struct UIEvent;
class UIObject; // any item
class UIElement; // physical item
class UINode; // logical item
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
class System;
}


using UIRect = AABB<float>;

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
};

enum class Direction
{
	Horizontal,
	Vertical,
};

class UIObject
{
public:
	virtual ~UIObject();
	virtual void Reset() {}
	virtual void OnInit() {}
	virtual void OnDestroy() {}
	virtual void OnCompleteStructure() {}
	virtual void OnEvent(UIEvent& e) {}
	virtual void OnPaint()
	{
		PaintChildren();
	}
	void PaintChildren()
	{
		for (auto* ch = firstChild; ch; ch = ch->next)
			ch->OnPaint();
	}
	virtual float CalcEstimatedWidth(float containerWidth, float containerHeight);
	virtual float CalcEstimatedHeight(float containerWidth, float containerHeight);
	float GetEstimatedWidth(float containerWidth, float containerHeight);
	float GetEstimatedHeight(float containerWidth, float containerHeight);
	virtual float GetFullEstimatedWidth(float containerWidth, float containerHeight);
	virtual float GetFullEstimatedHeight(float containerWidth, float containerHeight);
	virtual void OnLayout(const UIRect& rect);
	virtual bool Contains(float x, float y) const
	{
		return GetBorderRect().Contains(x, y);
	}
	bool IsChildOf(UIObject* obj) const;
	bool IsChildOrSame(UIObject* obj) const;
	int CountChildElements() const;
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

	bool IsHovered() const;
	bool IsClicked(int button = 0) const;
	bool IsFocused() const;

	bool IsInputDisabled() const;
	void SetInputDisabled(bool v);

	style::Accessor GetStyle();
	float ResolveUnits(style::Coord coord, float ref);

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

	ui::System* system = nullptr;
	style::Block* styleProps = nullptr;

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

class UITextElement : public UIElement
{
public:
	UITextElement()
	{
		GetStyle().SetLayout(style::Layout::InlineBlock);
	}
	float CalcEstimatedWidth(float containerWidth, float containerHeight) override
	{
		return GetTextWidth(text.c_str());
	}
	float CalcEstimatedHeight(float containerWidth, float containerHeight) override
	{
		return GetFontHeight();
	}
	void OnLayout(const UIRect& rect)
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
		UIElement::OnPaint();
	}
	void SetText(const char* t)
	{
		text = t;
	}

	std::string text;
};

class UIBoxElement : public UIElement
{
};

class UINode : public UIObject
{
public:
	typedef char IsNode[2];

	virtual void Render(UIContainer* ctx) = 0;
	void Rerender();
};
