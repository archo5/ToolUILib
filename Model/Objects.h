
#pragma once
#pragma warning(disable:4996)
#pragma warning(disable:4244)
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <typeinfo>
#include <atomic>
#include <new>

#include "../Core/Math.h"
#include "../Core/Serialization.h"
#include "../Core/String.h"

#include "../Render/Render.h"

#include "Events.h"
#include "Layout.h"


struct UIEvent;
struct UIObject; // any item
struct UIElement; // physical item
struct UIContainer;
struct UIEventSystem;

namespace prs {
class PropertyBlock;
enum class Unit;
}

namespace style {
struct Block;
}

namespace ui {
struct NativeWindowBase;
class FrameContents;
struct EventHandlerEntry;
struct Node; // logical item

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
	UIObject_IsEdited = 1 << 11,
};

enum class Direction
{
	Horizontal,
	Vertical,
};

struct LivenessToken
{
	struct Data
	{
		std::atomic<int32_t> ref;
		std::atomic_bool alive;
	};

	Data* _data;

	LivenessToken() : _data(nullptr) {}
	LivenessToken(Data* d) : _data(d) { if (d) d->ref++; }
	LivenessToken(const LivenessToken& o) : _data(o._data) { if (_data) _data->ref++; }
	LivenessToken(LivenessToken&& o) : _data(o._data) { o._data = nullptr; }
	~LivenessToken() { Release(); }
	LivenessToken& operator = (const LivenessToken& o)
	{
		Release();
		_data = o._data;
		if (_data)
			_data->ref++;
		return *this;
	}
	LivenessToken& operator = (LivenessToken&& o)
	{
		Release();
		_data = o._data;
		o._data = nullptr;
		return *this;
	}
	void Release()
	{
		if (_data && --_data->ref <= 0)
		{
			delete _data;
			_data = nullptr;
		}
	}

	bool IsAlive() const
	{
		return _data && _data->alive;
	}
	void SetAlive(bool alive)
	{
		if (_data)
			_data->alive = alive;
	}
	LivenessToken& GetOrCreate()
	{
		if (!_data)
			_data = new Data{ 1, true };
		return *this;
	}
};

struct IDataSerializer
{
	virtual void Process(void* data, size_t size) = 0;
	virtual bool IsWriter() = 0;

	template <class T> IDataSerializer& operator << (T& val)
	{
		Process(&val, sizeof(val));
		return *this;
	}
};

struct DataWriteSerializer : IDataSerializer
{
	DataWriteSerializer(char* _p) : p(_p) {}
	void Process(void* data, size_t size) override
	{
		memcpy(p, data, size);
		p += size;
	}
	bool IsWriter() override { return true; }

	char* p;
};

struct DataReadSerializer : IDataSerializer
{
	DataReadSerializer(char* _p) : p(_p) {}
	void Process(void* data, size_t size) override
	{
		memcpy(data, p, size);
		p += size;
	}
	bool IsWriter() override { return false; }

	char* p;
};

struct UIObject
{
	UIObject();
	virtual ~UIObject();
	virtual void OnInit() {}
	virtual void OnDestroy() {}
	virtual void OnCompleteStructure() {}

	virtual void OnSerialize(IDataSerializer&) {}
	void _SerializePersistent(IDataSerializer& s);

	virtual void OnEvent(UIEvent& e) {}
	void _DoEvent(UIEvent& e);

	ui::EventFunc& HandleEvent(UIEventType type) { return HandleEvent(nullptr, type); }
	ui::EventFunc& HandleEvent(UIObject* target = nullptr, UIEventType type = UIEventType::Any);
	void ClearEventHandlers(bool localOnly = false);

	virtual void OnPaint();
	void Paint();
	void PaintChildren()
	{
		GL::PushScissorRect(finalRectC.x0, finalRectC.y0, finalRectC.x1, finalRectC.y1);
		for (auto* ch = firstChild; ch; ch = ch->next)
			ch->Paint();
		GL::PopScissorRect();
	}
	virtual void GetSize(style::Coord& outWidth, style::Coord& outHeight) {}
	virtual float CalcEstimatedWidth(const Size<float>& containerSize, style::EstSizeType type);
	virtual float CalcEstimatedHeight(const Size<float>& containerSize, style::EstSizeType type);
	Range<float> GetEstimatedWidth(const Size<float>& containerSize, style::EstSizeType type);
	Range<float> GetEstimatedHeight(const Size<float>& containerSize, style::EstSizeType type);
	virtual Range<float> GetFullEstimatedWidth(const Size<float>& containerSize, style::EstSizeType type);
	virtual Range<float> GetFullEstimatedHeight(const Size<float>& containerSize, style::EstSizeType type);
	void PerformLayout(const UIRect& rect, const Size<float>& containerSize);
	virtual void OnLayout(const UIRect& rect, const Size<float>& containerSize);
	virtual bool Contains(float x, float y) const
	{
		return GetBorderRect().Contains(x, y);
	}

	bool InUse() const { return !!(flags & UIObject_IsClickedAnyMask) || IsFocused(); }
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
	LivenessToken GetLivenessToken() { return _livenessToken.GetOrCreate(); }

	void dump() { printf("    [=%p ]=%p ^=%p <=%p >=%p\n", firstChild, lastChild, parent, prev, next); fflush(stdout); }

	uint32_t flags = 0;
	UIObject* parent = nullptr;
	UIObject* firstChild = nullptr;
	UIObject* lastChild = nullptr;
	UIObject* prev = nullptr;
	UIObject* next = nullptr;

	ui::EventHandlerEntry* _firstEH = nullptr;
	ui::EventHandlerEntry* _lastEH = nullptr;

	ui::FrameContents* system = nullptr;
	style::BlockRef styleProps;
	LivenessToken _livenessToken;

	// final layout rectangles: C=content, P=padding, B=border
	UIRect finalRectC = {};
	UIRect finalRectCP = {};
	UIRect finalRectCPB = {};

	// size cache
	uint32_t _cacheFrameWidth = {};
	uint32_t _cacheFrameHeight = {};
	Range<float> _cacheValueWidth = {};
	Range<float> _cacheValueHeight = {};
};

struct UIElement : UIObject
{
	typedef char IsElement[1];
};

namespace ui {

struct TextElement : UIElement
{
	TextElement();
#if 0
	float CalcEstimatedWidth(const Size<float>& containerSize, style::EstSizeType type) override
	{
		return ceilf(GetTextWidth(text.c_str()));
	}
	float CalcEstimatedHeight(const Size<float>& containerSize, style::EstSizeType type) override
	{
		return GetFontHeight();
	}
	void OnLayout(const UIRect& rect, const Size<float>& containerSize) override
	{
		finalRectCP = finalRectCPB = rect;
		finalRectC = finalRectCP.ShrinkBy(GetPaddingRect(styleProps, rect.GetWidth()));
		//finalRect.x1 = finalRect.x0 + GetTextWidth(text)
		//finalRect.y1 = finalRect.y0 + GetFontHeight();
	}
#endif
	void GetSize(style::Coord& outWidth, style::Coord& outHeight) override;
	void OnPaint() override;

	void SetText(StringView t)
	{
		text.assign(t.data(), t.size());
	}

	std::string text;
};

struct BoxElement : UIElement
{
};

struct Subscription;
struct DataCategoryTag {};

constexpr auto ANY_ITEM = uintptr_t(-1);

void Notify(DataCategoryTag* tag, uintptr_t at = ANY_ITEM);
inline void Notify(DataCategoryTag* tag, const void* ptr)
{
	Notify(tag, reinterpret_cast<uintptr_t>(ptr));
}

struct Node : UIObject
{
	~Node();
	typedef char IsNode[2];

	virtual void Render(UIContainer* ctx) = 0;
	void Rerender();

	virtual void OnNotify(DataCategoryTag* tag, uintptr_t at);
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

	Subscription* _firstSub = nullptr;
	Subscription* _lastSub = nullptr;
};

struct Modifier
{
	virtual void Apply(UIObject* obj) const = 0;
};

inline UIObject& operator + (UIObject& o, const Modifier& m)
{
	m.Apply(&o);
	return o;
}

struct Enable : Modifier
{
	bool _enable;
	Enable(bool e) : _enable(e) {}
	void Apply(UIObject* obj) const override { obj->SetInputDisabled(!_enable); }
};

struct Layout : Modifier
{
	style::Layout* _layout;
	Layout(style::Layout* layout) : _layout(layout) {}
	void Apply(UIObject* obj) const override { obj->GetStyle().SetLayout(_layout); }
};

struct StackingDirection : Modifier
{
	style::StackingDirection _dir;
	StackingDirection(style::StackingDirection dir) : _dir(dir) {}
	void Apply(UIObject* obj) const override { obj->GetStyle().SetStackingDirection(_dir); }
};

struct BoxSizing : Modifier
{
	style::BoxSizing _bs;
	BoxSizing(style::BoxSizing bs) : _bs(bs) {}
	void Apply(UIObject* obj) const override { obj->GetStyle().SetBoxSizing(_bs); }
};

struct Width : Modifier
{
	style::Coord _c;
	Width(const style::Coord& c) : _c(c) {}
	void Apply(UIObject* obj) const override { obj->GetStyle().SetWidth(_c); }
};

struct Height : Modifier
{
	style::Coord _c;
	Height(const style::Coord& c) : _c(c) {}
	void Apply(UIObject* obj) const override { obj->GetStyle().SetHeight(_c); }
};

struct MinWidth : Modifier
{
	style::Coord _c;
	MinWidth(const style::Coord& c) : _c(c) {}
	void Apply(UIObject* obj) const override { obj->GetStyle().SetMinWidth(_c); }
};

struct Padding : Modifier
{
	style::Coord _l, _r, _t, _b;
	Padding(const style::Coord& c) : _l(c), _r(c), _t(c), _b(c) {}
	void Apply(UIObject* obj) const override { obj->GetStyle().SetPadding(_t, _r, _b, _l); }
};

struct EventHandler : Modifier
{
	std::function<void(UIEvent&)> _evfn;
	UIEventType _type = UIEventType::Any;
	UIObject* _tgt = nullptr;
	EventHandler(std::function<void(UIEvent&)>&& fn) : _evfn(std::move(fn)) {}
	EventHandler(UIEventType t, std::function<void(UIEvent&)>&& fn) : _evfn(std::move(fn)), _type(t) {}
	void Apply(UIObject* obj) const override { obj->HandleEvent(_tgt, _type) = std::move(_evfn); }
};

} // ui
