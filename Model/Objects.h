
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
#include "Painting.h"


namespace ui {

struct Event;
struct UIObject; // any item
struct UIContainer;
struct EventSystem;

struct NativeWindowBase;
struct FrameContents;
struct EventHandlerEntry;
struct Buildable; // logical item

using EventFunc = std::function<void(Event& e)>;


template <class T>
struct TempEditable
{
	using Type = T;

	Type* _current;
	Type _backup;

	UI_FORCEINLINE Type* operator -> () const { return _current; }
	UI_FORCEINLINE TempEditable(Type* cur) : _current(cur), _backup(*cur) {}
	UI_FORCEINLINE TempEditable(TempEditable&& o) : _current(o._current), _backup(o._backup) { o._current = nullptr; }
	TempEditable(const TempEditable&) = delete;
	UI_FORCEINLINE TempEditable& operator = (TempEditable&& o)
	{
		if (this == &o)
			return *this;
		_current = o._current;
		_backup = o._backup;
		o._current = nullptr;
		return *this;
	}
	TempEditable& operator = (const TempEditable& o) = delete;
	UI_FORCEINLINE ~TempEditable()
	{
		if (_current)
			*_current = _backup;
	}
	UI_FORCEINLINE void Revert()
	{
		*_current = _backup;
	}
};


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
	//UIObject_ClipChildren = 1 << 26,
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
	IPersistentObject* _next = nullptr; // @ 0p1

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
	UIPaintContext WithAdvice(const ContentPaintAdvice& cpa) const
	{
		UIPaintContext pc = *this;
		if (cpa.HasTextColor())
			pc.textColor = cpa.GetTextColor();
		return pc;
	}
};

struct SingleChildPaintPtr
{
	SingleChildPaintPtr* next;
	UIObject* child;
};

struct UIObjectIteratorData
{
	uintptr_t data0;
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

	virtual void _AttachToFrameContents(FrameContents* owner);
	virtual void _DetachFromFrameContents();
	virtual void _DetachFromTree();

	void PO_ResetConfiguration() override;
	void PO_BeforeDelete() override;
	void _InitReset();
	virtual void OnReset() {}

	virtual void SlotIterator_Init(UIObjectIteratorData& data) = 0;
	virtual UIObject* SlotIterator_GetNext(UIObjectIteratorData& data) = 0;

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

	virtual void OnPaint(const UIPaintContext& ctx) = 0;
	void Paint(const UIPaintContext& ctx);
	void RootPaint();
	virtual void OnPaintSingleChild(SingleChildPaintPtr* next, const UIPaintContext& ctx);

	void PerformLayout(const UIRect& rect);
	virtual Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) = 0;
	virtual Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) = 0;
	virtual void OnLayout(const UIRect& rect) = 0;
	virtual void OnLayoutChanged() {}
	virtual void RedoLayout();

	virtual bool Contains(Point2f pos) const
	{
		return GetFinalRect().Contains(pos);
	}
	virtual Point2f LocalToChildPoint(Point2f pos) const { return pos; }
	virtual UIObject* FindLastChildContainingPos(Point2f pos) const = 0;

	void SetFlag(UIObjectFlags flag, bool set);
	static bool HasFlags(uint32_t total, UIObjectFlags f) { return (total & f) == f; }
	bool HasFlags(UIObjectFlags f) const { return (flags & f) == f; }
	bool InUse() const { return !!(flags & UIObject_IsClickedAnyMask) || IsFocused(); }
	bool _CanPaint() const { return !(flags & (UIObject_IsHidden | UIObject_IsOverlay | UIObject_NoPaint)); }
	bool _NeedsLayout() const { return !(flags & UIObject_IsHidden); }

	virtual void RemoveChildImpl(UIObject* ch) = 0;
	void DetachAll();
	void DetachParent();
	virtual void DetachChildren(bool recursive = false) = 0;
	virtual void CustomAppendChild(UIObject* obj) = 0;

	bool IsChildOf(UIObject* obj) const;
	bool IsChildOrSame(UIObject* obj) const;
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

	// for focus (lower priority) navigation (can be slower)
	UIObject* LPN_GetFirstInForwardOrder() { return this; }
	UIObject* LPN_GetNextInForwardOrder();
	UIObject* LPN_GetPrevInReverseOrder();
	UIObject* LPN_GetLastInReverseOrder();
	// - primitives for replacing generic iteration:
	virtual UIObject* LPN_GetFirstChild();
	virtual UIObject* LPN_GetChildAfter(UIObject* cur);
	virtual UIObject* LPN_GetChildBefore(UIObject* cur);
	virtual UIObject* LPN_GetLastChild();

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

	void _OnChangeStyle();

	float ResolveUnits(Coord coord, float ref);

	UI_FORCEINLINE UIRect GetFinalRect() const { return _finalRect; }

	UI_FORCEINLINE const FontSettings* FindFontSettings(const FontSettings* first) const { return first ? first : _FindClosestParentFontSettings(); }
	const FontSettings* _FindClosestParentFontSettings() const;
	virtual const FontSettings* _GetFontSettings() const;

	ui::NativeWindowBase* GetNativeWindow() const;
	LivenessToken GetLivenessToken() { return _livenessToken.GetOrCreate(); }

	uint32_t flags = UIObject_DB__Defaults; // @ 0p2
	uint32_t _pendingDeactivationSetPos = UINT32_MAX; // @ 4p2
	ui::FrameContents* system = nullptr; // @ 8p2
	UIObject* parent = nullptr; // @ 8p3

	ui::EventHandlerEntry* _firstEH = nullptr; // @ 8p4
	ui::EventHandlerEntry* _lastEH = nullptr; // @ 8p5

	LivenessToken _livenessToken; // @ 8p6

	// final layout rectangle
	UIRect _finalRect = {}; // @ 8p7

	// total size: 24p7 (52/80)

	// TODO
	UIObject* next = nullptr;
	UIObject* prev = nullptr;
};

struct UIObjectIterator
{
	UIObject* _obj;
	UIObjectIteratorData _data;

	UIObjectIterator(UIObject* o) : _obj(o)
	{
		o->SlotIterator_Init(_data);
	}
	UIObject* GetNext()
	{
		return _obj->SlotIterator_GetNext(_data);
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

struct UIObjectLegacyChildren : UIObject
{
	UIObject* firstChild = nullptr;
	UIObject* lastChild = nullptr;

	void _AttachToFrameContents(FrameContents* owner) override;
	void _DetachFromFrameContents() override;
	void _DetachFromTree() override;

	void SlotIterator_Init(UIObjectIteratorData& data) override;
	UIObject* SlotIterator_GetNext(UIObjectIteratorData& data) override;

	void OnPaint(const UIPaintContext& ctx) override;

	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect) override;

	UIObject* FindLastChildContainingPos(Point2f pos) const override;

	void RemoveChildImpl(UIObject* ch) override;
	void DetachChildren(bool recursive = false) override;
	void CustomAppendChild(UIObject* obj) override;
};

struct UIObjectNoChildren : UIObject
{
	void SlotIterator_Init(UIObjectIteratorData& data) override {}
	UIObject* SlotIterator_GetNext(UIObjectIteratorData& data) override { return nullptr; }
	void RemoveChildImpl(UIObject* ch) override {}
	void DetachChildren(bool recursive) override {}
	void CustomAppendChild(UIObject* obj) override;
	void OnPaint(const UIPaintContext& ctx) override {}
	UIObject* FindLastChildContainingPos(Point2f pos) const override { return nullptr; }

	void OnLayout(const UIRect& rect) override { _finalRect = rect; }
};

struct UIObjectSingleChild : UIObject
{
	UIObject* _child = nullptr;

	void OnReset() override;
	void SlotIterator_Init(UIObjectIteratorData& data) override;
	UIObject* SlotIterator_GetNext(UIObjectIteratorData& data) override;
	void RemoveChildImpl(UIObject* ch) override;
	void DetachChildren(bool recursive) override;
	void CustomAppendChild(UIObject* obj) override;
	void OnPaint(const UIPaintContext& ctx) override;
	UIObject* FindLastChildContainingPos(Point2f pos) const override;
	void _AttachToFrameContents(FrameContents* owner) override;
	void _DetachFromFrameContents() override;
	void _DetachFromTree() override;
};

// TODO: slowly port to these
struct WrapperElement : UIObjectSingleChild
{
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect) override;
};

struct FillerElement : UIObjectSingleChild
{
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override
	{
		return Rangef::Exact(containerSize.x);
	}
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override
	{
		return Rangef::Exact(containerSize.y);
	}
	void OnLayout(const UIRect& rect) override
	{
		if (_child && _child->_NeedsLayout())
			_child->PerformLayout(rect);
		_finalRect = rect;
	}
};

struct SizeConstraintElement : WrapperElement
{
	Rangef widthRange = Rangef::AtLeast(0);
	Rangef heightRange = Rangef::AtLeast(0);

	void OnReset() override;
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;

	SizeConstraintElement& SetWidthRange(Rangef r) { widthRange = r; return *this; }
	SizeConstraintElement& SetHeightRange(Rangef r) { heightRange = r; return *this; }

	SizeConstraintElement& SetMinWidth(float w) { widthRange.min = w; return *this; }
	SizeConstraintElement& SetMaxWidth(float w) { widthRange.max = w; return *this; }
	SizeConstraintElement& SetMinHeight(float h) { heightRange.min = h; return *this; }
	SizeConstraintElement& SetMaxHeight(float h) { heightRange.max = h; return *this; }

	SizeConstraintElement& SetWidth(float w) { widthRange.min = w; widthRange.max = w; return *this; }
	SizeConstraintElement& SetHeight(float h) { heightRange.min = h; heightRange.max = h; return *this; }
	SizeConstraintElement& SetSize(float w, float h)
	{
		widthRange.min = w;
		widthRange.max = w;
		heightRange.min = h;
		heightRange.max = h;
		return *this;
	}
	SizeConstraintElement& SetSize(Size2f s)
	{
		widthRange.min = s.x;
		widthRange.max = s.x;
		heightRange.min = s.y;
		heightRange.max = s.y;
		return *this;
	}
};

struct TextElement : UIObjectNoChildren
{
	std::string text;

	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;

	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override
	{
		auto* fs = _FindClosestParentFontSettings();
		return Rangef::Exact(ceilf(GetTextWidth(fs->GetFont(), fs->size, text)));
	}
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override
	{
		auto* fs = _FindClosestParentFontSettings();
		return Rangef::Exact(fs->size);
	}

	TextElement& SetText(StringView t)
	{
		text.assign(t.data(), t.size());
		return *this;
	}
};

struct Placeholder : UIObject
{
	void OnPaint(const UIPaintContext& ctx) override;
};

struct ChildScaleOffsetElement : WrapperElement
{
	ScaleOffset2D transform;

	Size2f _childSize;
	draw::VertexTransformCallback _prevVTCB;

	void OnReset() override;

	static void TransformVerts(void* userdata, Vertex* vertices, size_t count);
	void OnPaint(const UIPaintContext& ctx) override;
	void OnPaintSingleChild(SingleChildPaintPtr* next, const UIPaintContext& ctx) override;

	Point2f LocalToChildPoint(Point2f pos) const override;
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect) override;
};

struct Subscription;
struct DataCategoryTag {};

constexpr auto ANY_ITEM = uintptr_t(-1);

void Notify(DataCategoryTag* tag, uintptr_t at = ANY_ITEM);
inline void Notify(DataCategoryTag* tag, const void* ptr)
{
	Notify(tag, reinterpret_cast<uintptr_t>(ptr));
}

struct Buildable : UIObjectLegacyChildren
{
	~Buildable();
	typedef char IsBuildable[2];

	void PO_ResetConfiguration() override;

	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect) override;

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
#define WRAPPER 1
#define FILLER 2
	uint8_t TEMP_LAYOUT_MODE = 0;
};


struct Modifier
{
	virtual void OnBeforeControlGroup() const {}
	virtual void OnAfterControlGroup() const {}
	virtual void OnBeforeControl() const {}
	virtual void OnAfterControl() const {}
	virtual void OnBeforeContent() const {}
	virtual void OnAfterContent() const {}
	virtual void Apply(UIObject* obj) const {}
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

#undef UI_COORD_VALUE_PROXY

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
