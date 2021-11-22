
#pragma once
#pragma warning(disable:4996)
#pragma warning(disable:4244)
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <typeinfo>
#include <new>

#include "../Core/Math.h"
#include "../Core/Serialization.h"
#include "../Core/String.h"
#include "../Core/Threading.h"
#include "../Core/Font.h"

#include "../Render/Render.h"

#include "Events.h"
#include "Layout.h"


namespace ui {

struct Event;
struct UIObject; // any item
struct UIElement; // physical item
struct UIContainer;
struct EventSystem;

struct NativeWindowBase;
struct FrameContents;
struct EventHandlerEntry;
struct Buildable; // logical item

using EventFunc = std::function<void(Event& e)>;


enum UIObjectFlags
{
	UIObject_IsInBuildStack = 1 << 0,
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
	UIObject_IsChecked = 1 << 12,
	UIObject_IsFocusable = 1 << 13,
	UIObject_IsPressedMouse = 1 << 14,
	UIObject_IsPressedOther = 1 << 15,
	UIObject_IsPressedAny = UIObject_IsPressedMouse | UIObject_IsPressedOther,
	UIObject_DB_IMEdit = 1 << 16, // +IsEdited and Rebuild upon activation
	UIObject_IsOverlay = 1 << 17,
	UIObject_DB_CaptureMouseOnLeftClick = 1 << 18,
	UIObject_DB_FocusOnLeftClick = UIObject_IsFocusable | (1 << 19),
	UIObject_DB_Button = UIObject_DB_CaptureMouseOnLeftClick | UIObject_DB_FocusOnLeftClick | (1 << 20),
	UIObject_DB_Draggable = UIObject_DB_CaptureMouseOnLeftClick | (1 << 21),
	UIObject_DB_Selectable = 1 << 22,
	UIObject_DisableCulling = 1 << 23,
	UIObject_NoPaint = 1 << 24,
	UIObject_DB_RebuildOnChange = 1 << 25,
	UIObject_ClipChildren = 1 << 26,
	UIObject_IsInTree = 1 << 27,
	UIObject_NeedsTreeUpdates = 1 << 28,
	UIObject_SetsChildTextStyle = 1 << 29,

	UIObject_DB__Defaults = 0,
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
		AtomicInt32 ref;
		AtomicBool alive;
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

// objects whose configuration can be reset without also resetting the state
struct IPersistentObject
{
	IPersistentObject* _next = nullptr;

	virtual ~IPersistentObject() {}
	virtual void PO_ResetConfiguration() {}
	virtual void PO_BeforeDelete() {}
};

inline void DeletePersistentObject(IPersistentObject* obj)
{
	if (obj)
	{
		obj->PO_BeforeDelete();
		delete obj;
	}
}

struct PersistentObjectList
{
	IPersistentObject* _firstPO = nullptr;
	IPersistentObject** _curPO = nullptr;

	~PersistentObjectList()
	{
		DeleteAll();
	}

	void DeleteAll()
	{
		_curPO = &_firstPO;
		DeleteRemaining();
		_firstPO = nullptr;
	}

	void BeginAllocations()
	{
		_curPO = &_firstPO;
		// reset all
		for (auto* cur = _firstPO; cur; cur = cur->_next)
			cur->PO_ResetConfiguration();
	}

	void EndAllocations()
	{
		DeleteRemaining();
		_curPO = nullptr;
	}

	void DeleteRemaining()
	{
		IPersistentObject* next;
		for (auto* cur = *_curPO; cur; cur = next)
		{
			next = cur->_next;
			DeletePersistentObject(cur);
		}
		*_curPO = nullptr;
	}

	template <class T> T* TryNext()
	{
		auto*& cur = *_curPO;
		if (cur && typeid(*cur) == typeid(T))
		{
			// already reset in BeginAllocations
			//cur->PO_ResetConfiguration();
			_curPO = &cur->_next;
			return static_cast<T*>(cur);
		}
		return nullptr;
	}

	void AddNext(IPersistentObject* po)
	{
		DeleteRemaining();
		*_curPO = po;
		_curPO = &po->_next;
	}
};

template <class T>
struct UIWeakPtr
{
	LivenessToken _token;
	T* _obj = nullptr;

	UI_FORCEINLINE UIWeakPtr() {}
	UI_FORCEINLINE UIWeakPtr(T* o) : _obj(o) { if (o) _token = o->GetLivenessToken(); }
	UI_FORCEINLINE T* Get() const { return _token.IsAlive() ? _obj : nullptr; }
	UI_FORCEINLINE T* operator -> () const { return Get(); }
	UI_FORCEINLINE operator T* () const { return Get(); }
};

struct UIPaintContext
{
	Color4b textColor;

	UI_FORCEINLINE UIPaintContext() {}
	UI_FORCEINLINE UIPaintContext(DoNotInitialize) : textColor(DoNotInitialize{}) {}
	UIPaintContext(const UIPaintContext&, Color4b arg_textColor)
	{
		textColor = arg_textColor;
	}
};

struct UIObject : IPersistentObject
{
	UIObject();
	// prevent accidental copies
	UIObject(const UIObject&) = delete;
	virtual ~UIObject();

	virtual void OnEnable() {}
	virtual void OnDisable() {}
	virtual void OnEnterTree() {}
	virtual void OnExitTree() {}

	void _AttachToFrameContents(FrameContents* owner);
	void _DetachFromFrameContents();
	void _DetachFromTree();

	void PO_ResetConfiguration() override;
	void PO_BeforeDelete() override;
	void _InitReset();
	virtual void OnReset() {}

	virtual void OnEvent(Event& e) {}
	void _DoEvent(Event& e);
	void _PerformDefaultBehaviors(Event& e, uint32_t f);

	void SendUserEvent(int id, uintptr_t arg0 = 0, uintptr_t arg1 = 0);
	ui::EventFunc& HandleEvent(EventType type) { return HandleEvent(nullptr, type); }
	ui::EventFunc& HandleEvent(UIObject* target = nullptr, EventType type = EventType::Any);
	void ClearEventHandlers();
	void ClearLocalEventHandlers();

	void RegisterAsOverlay(float depth = 0);
	void UnregisterAsOverlay();

	virtual void OnPaint(const UIPaintContext& ctx);
	void Paint(const UIPaintContext& ctx);
	void PaintChildren(const UIPaintContext& ctx, const ContentPaintAdvice& cpa);
	virtual void GetSize(Coord& outWidth, Coord& outHeight) {}
	virtual float CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type);
	virtual float CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type);
	Rangef GetEstimatedWidth(const Size2f& containerSize, EstSizeType type);
	Rangef GetEstimatedHeight(const Size2f& containerSize, EstSizeType type);
	virtual Rangef GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type, bool forParentLayout = true);
	virtual Rangef GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type, bool forParentLayout = true);
	void PerformLayout(const UIRect& rect, const Size2f& containerSize);
	void _PerformPlacement(const UIRect& rect, const Size2f& containerSize);
	virtual void OnLayoutChanged() {}
	virtual void OnLayout(const UIRect& rect, const Size2f& containerSize);
	virtual UIRect CalcPaddingRect(const UIRect& expTgtRect);

	virtual bool Contains(Point2f pos) const
	{
		return GetBorderRect().Contains(pos);
	}
	virtual Point2f LocalToChildPoint(Point2f pos) const { return pos; }

	void SetFlag(UIObjectFlags flag, bool set);
	static bool HasFlags(uint32_t total, UIObjectFlags f) { return (total & f) == f; }
	bool HasFlags(UIObjectFlags f) const { return (flags & f) == f; }
	bool InUse() const { return !!(flags & UIObject_IsClickedAnyMask) || IsFocused(); }
	bool _CanPaint() const { return !(flags & (UIObject_IsHidden | UIObject_IsOverlay | UIObject_NoPaint)); }
	bool _NeedsLayout() const { return !(flags & UIObject_IsHidden); }
	bool _IsPartOfParentLayout() { return !(flags & UIObject_IsHidden) && (!GetStyle().GetPlacement() || GetStyle().GetPlacement()->applyOnLayout); }

	void DetachAll();
	void DetachParent();
	void DetachChildren(bool recursive = false);
	void InsertPrevious(UIObject* obj);
	void InsertNext(UIObject* obj);
	void PrependChild(UIObject* obj);
	void AppendChild(UIObject* obj);
	void _AddFirstChild(UIObject* obj);

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
	UIObject* GetPrevInOrder();
	UIObject* GetNextInOrder();
	UIObject* GetFirstInOrder();
	UIObject* GetLastInOrder();

	void Rebuild();
	void RebuildContainer();
	void _OnIMChange();

	bool IsHovered() const;
	bool IsPressed() const;
	bool IsClicked(int button = 0) const;
	bool IsFocused() const;

	bool IsVisible() const;
	void SetVisible(bool v);

	bool IsInputDisabled() const;
	void SetInputDisabled(bool v);

	StyleAccessor GetStyle();
	void SetStyle(StyleBlock* style);
	void _OnChangeStyle();

	float ResolveUnits(Coord coord, float ref);

	UIRect GetContentRect() const { return finalRectC; }
	UIRect GetPaddingRect() const { return finalRectCP; }
	UIRect GetBorderRect() const { return finalRectCPB; }

	UI_FORCEINLINE StyleBlock* FindTextStyle(StyleBlock* first) const { return first ? first : _FindClosestParentTextStyle(); }
	StyleBlock* _FindClosestParentTextStyle() const;

	ui::NativeWindowBase* GetNativeWindow() const;
	LivenessToken GetLivenessToken() { return _livenessToken.GetOrCreate(); }

	void dump() { printf("    [=%p ]=%p ^=%p <=%p >=%p\n", firstChild, lastChild, parent, prev, next); fflush(stdout); }

	uint32_t flags = UIObject_DB__Defaults; // @ 16
	uint32_t _pendingDeactivationSetPos = UINT32_MAX; // @ 20
	ui::FrameContents* system = nullptr; // @ 24
	UIObject* parent = nullptr; // @ 32
	UIObject* firstChild = nullptr; // @ 40
	UIObject* lastChild = nullptr; // @ 48
	UIObject* next = nullptr; // @56
	UIObject* prev = nullptr; // @64

	ui::EventHandlerEntry* _firstEH = nullptr;
	ui::EventHandlerEntry* _lastEH = nullptr;

	StyleBlockRef styleProps;
	LivenessToken _livenessToken;

	// final layout rectangles: C=content, P=padding, B=border
	UIRect finalRectC = {};
	UIRect finalRectCP = {};
	UIRect finalRectCPB = {};

	// previous layout input argument cache
	UIRect lastLayoutInputRect = {};
	Size2f lastLayoutInputCSize = {};

	// size cache
	uint32_t _cacheFrameWidth = {};
	uint32_t _cacheFrameHeight = {};
	Rangef _cacheValueWidth = { 0, 0 };
	Rangef _cacheValueHeight = { 0, 0 };
};

struct UIPaintHelper
{
	ContentPaintAdvice cpa;

	void PaintBackground(UIObject* obj)
	{
		if (obj->styleProps->background_painter)
			cpa = obj->styleProps->background_painter->Paint(obj);
	}
	void PaintBackground(const PaintInfo& info)
	{
		if (info.obj->styleProps->background_painter)
			cpa = info.obj->styleProps->background_painter->Paint(info);
	}

	void PaintChildren(UIObject* obj, const UIPaintContext& ctx)
	{
		obj->PaintChildren(ctx, cpa);
	}

	static void Paint(const PaintInfo& info, const UIPaintContext& ctx)
	{
		UIPaintHelper ph;
		ph.PaintBackground(info);
		ph.PaintChildren(const_cast<UIObject*>(info.obj), ctx);
	}
};

template <class T> inline T* CreateUIObject()
{
	auto* obj = new T;
	obj->_InitReset();
	return obj;
}

inline void DeleteUIObject(UIObject* obj)
{
	DeletePersistentObject(obj);
}

struct UIElement : UIObject
{
	typedef char IsElement[1];
};

struct TextElement : UIElement
{
	std::string text;

	void OnReset() override;
#if 0
	float CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override
	{
		return ceilf(GetTextWidth(styleProps->GetFont(), styleProps->font_size, text));
	}
	float CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override
	{
		return styleProps->font_size;
	}
	void OnLayout(const UIRect& rect, const Size2f& containerSize) override
	{
		finalRectCP = finalRectCPB = rect;
		finalRectC = finalRectCP.ShrinkBy(GetPaddingRect(styleProps, rect.GetWidth()));
		//finalRect.x1 = finalRect.x0 + GetTextWidth(styleProps->GetFont(), styleProps->font_size, text)
		//finalRect.y1 = finalRect.y0 + styleProps->font_size;
	}
#endif
	void GetSize(Coord& outWidth, Coord& outHeight) override;
	void OnPaint(const UIPaintContext& ctx) override;

	TextElement& SetText(StringView t)
	{
		text.assign(t.data(), t.size());
		return *this;
	}
};

struct BoxElement : UIElement
{
};

struct Placeholder : UIElement
{
	void OnPaint(const UIPaintContext& ctx) override;
};

struct ChildScaleOffsetElement : UIElement
{
	float x, y, scale;

	ui::Size2f _childSize;
	ui::draw::VertexTransformCallback _prevVTCB;

	void OnReset() override;

	static void TransformVerts(void* userdata, ui::Vertex* vertices, size_t count);
	void OnPaint(const ui::UIPaintContext& ctx) override;

	ui::Point2f LocalToChildPoint(ui::Point2f pos) const override;
	float CalcEstimatedWidth(const ui::Size2f& containerSize, ui::EstSizeType type) override;
	float CalcEstimatedHeight(const ui::Size2f& containerSize, ui::EstSizeType type) override;
	void OnLayout(const ui::UIRect& rect, const ui::Size2f& containerSize) override;
};

struct Subscription;
struct DataCategoryTag {};

constexpr auto ANY_ITEM = uintptr_t(-1);

void Notify(DataCategoryTag* tag, uintptr_t at = ANY_ITEM);
inline void Notify(DataCategoryTag* tag, const void* ptr)
{
	Notify(tag, reinterpret_cast<uintptr_t>(ptr));
}

struct Buildable : UIObject
{
	~Buildable();
	typedef char IsBuildable[2];

	void PO_ResetConfiguration() override;

	virtual void Build() = 0;
	void Rebuild();

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

	void Defer(std::function<void()>&& fn)
	{
		_deferredDestructors.push_back(std::move(fn));
	}
	template <class T, class... Args> T* Allocate(Args&&... args)
	{
		T* obj = new T(std::forward<Args>(args)...);
		Defer([obj]() { delete obj; });
		return obj;
	}

	PersistentObjectList _objList;
	Subscription* _firstSub = nullptr;
	Subscription* _lastSub = nullptr;
	uint64_t _lastBuildFrameID = 0;
	std::vector<std::function<void()>> _deferredDestructors;
};


struct Modifier
{
	virtual void Apply(UIObject* obj) const = 0;
};

template <class T>
struct CRef
{
	CRef(const T& ref) : _ref(ref) {}
	operator const T& () const { return _ref; }
	const T* operator -> () const { return &_ref; }
	const T& operator * () const { return _ref; }
	const T& _ref;
};
using ModInitList = std::initializer_list<CRef<Modifier>>;

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

struct ApplyStyle : Modifier
{
	StyleBlock* _style;
	ApplyStyle(StyleBlock* style) : _style(style) {}
	void Apply(UIObject* obj) const override { obj->SetStyle(_style); }
};

struct SetLayout : Modifier
{
	ILayout* _layout;
	SetLayout(ILayout* layout) : _layout(layout) {}
	void Apply(UIObject* obj) const override { obj->GetStyle().SetLayout(_layout); }
};

struct SetPlacement : Modifier
{
	IPlacement* _placement;
	SetPlacement(IPlacement* placement) : _placement(placement) {}
	void Apply(UIObject* obj) const override { obj->GetStyle().SetPlacement(_placement); }
};

struct SetStackingDirection : Modifier
{
	StackingDirection _dir;
	SetStackingDirection(StackingDirection dir) : _dir(dir) {}
	void Apply(UIObject* obj) const override { obj->GetStyle().SetStackingDirection(_dir); }
};
inline SetStackingDirection Set(StackingDirection dir) { return dir; }

struct SetEdge : Modifier
{
	Edge _edge;
	SetEdge(Edge edge) : _edge(edge) {}
	void Apply(UIObject* obj) const override { obj->GetStyle().SetEdge(_edge); }
};
inline SetEdge Set(Edge edge) { return edge; }

struct SetBoxSizing : Modifier
{
	BoxSizing _bs;
	SetBoxSizing(BoxSizing bs) : _bs(bs) {}
	void Apply(UIObject* obj) const override { obj->GetStyle().SetBoxSizing(_bs); }
};
inline SetBoxSizing Set(BoxSizing bs) { return bs; }

#define UI_COORD_VALUE_PROXY(name) \
struct name : Modifier \
{ \
	Coord _c; \
	name(const Coord& c) : _c(c) {} \
	void Apply(UIObject* obj) const override { obj->GetStyle().name(_c); } \
}

UI_COORD_VALUE_PROXY(SetWidth);
UI_COORD_VALUE_PROXY(SetHeight);
UI_COORD_VALUE_PROXY(SetMinWidth);
UI_COORD_VALUE_PROXY(SetMinHeight);
UI_COORD_VALUE_PROXY(SetMaxWidth);
UI_COORD_VALUE_PROXY(SetMaxHeight);

#undef UI_COORD_VALUE_PROXY

struct SetPadding : Modifier
{
	float _l, _r, _t, _b;
	SetPadding(float c) : _l(c), _r(c), _t(c), _b(c) {}
	SetPadding(float v, float h) : _l(h), _r(h), _t(v), _b(v) {}
	SetPadding(float t, float lr, float b) : _l(lr), _r(lr), _t(t), _b(b) {}
	SetPadding(float t, float r, float b, float l) : _l(l), _r(r), _t(t), _b(b) {}
	void Apply(UIObject* obj) const override { obj->GetStyle().SetPadding(_t, _r, _b, _l); }
};

struct SetMargin : Modifier
{
	float _l, _r, _t, _b;
	SetMargin(float c) : _l(c), _r(c), _t(c), _b(c) {}
	SetMargin(float v, float h) : _l(h), _r(h), _t(v), _b(v) {}
	SetMargin(float t, float lr, float b) : _l(lr), _r(lr), _t(t), _b(b) {}
	SetMargin(float t, float r, float b, float l) : _l(l), _r(r), _t(t), _b(b) {}
	void Apply(UIObject* obj) const override { obj->GetStyle().SetMargin(_t, _r, _b, _l); }
};

struct AddEventHandler : Modifier
{
	std::function<void(Event&)> _evfn;
	EventType _type = EventType::Any;
	UIObject* _tgt = nullptr;
	AddEventHandler(std::function<void(Event&)>&& fn) : _evfn(std::move(fn)) {}
	AddEventHandler(EventType t, std::function<void(Event&)>&& fn) : _evfn(std::move(fn)), _type(t) {}
	void Apply(UIObject* obj) const override { if (_evfn) obj->HandleEvent(_tgt, _type) = std::move(_evfn); }
};

struct RebuildOnChange : Modifier
{
	void Apply(UIObject* obj) const override { obj->SetFlag(UIObject_DB_RebuildOnChange, true); }
};

struct AddTooltip : Modifier
{
	ui::Tooltip::BuildFunc _evfn;

	AddTooltip(ui::Tooltip::BuildFunc&& fn) : _evfn(std::move(fn)) {}
	AddTooltip(const std::string& s);
	void Apply(UIObject* obj) const override;
};

struct MakeOverlay : Modifier
{
	MakeOverlay(float depth = 0.0f) : _enable(true), _depth(depth) {}
	MakeOverlay(bool enable, float depth = 0.0f) : _enable(enable), _depth(depth) {}
	void Apply(UIObject* obj) const override
	{
		if (_enable)
			obj->RegisterAsOverlay(_depth);
		else
			obj->UnregisterAsOverlay();
	}

	bool _enable;
	float _depth;
};

struct MakeDraggable : AddEventHandler
{
	MakeDraggable() : AddEventHandler({}) {}
	MakeDraggable(std::function<void(Event&)>&& fn) : AddEventHandler(EventType::DragStart, std::move(fn)) {}
	MakeDraggable(std::function<void()>&& fn) : AddEventHandler(EventType::DragStart, [fn{ std::move(fn) }](Event&){ fn(); }) {}
	void Apply(UIObject* obj) const override
	{
		AddEventHandler::Apply(obj);
		obj->SetFlag(UIObjectFlags::UIObject_DB_Draggable, true);
	}
};

} // ui
