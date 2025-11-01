
#pragma once
#pragma warning(disable:4996)
#pragma warning(disable:4244)
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <typeinfo>

#include "../Core/Array.h"
#include "../Core/Math.h"
#include "../Core/String.h"
#include "../Core/Threading.h"
#include "../Core/WeakPtr.h"
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
	//UIObject_IsInBuildStack = 1 << 0,
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
	UIObject_HitTestPassthrough = 1 << 17,
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
	UIObject_AfterIMEdit = 1 << 30, // to queue event for next construction after the editing state was returned

	UIObject_DB__Defaults = 0,
};

enum class Direction : u8
{
	Horizontal,
	Vertical,
};

enum class Edge : u8
{
	Left,
	Top,
	Right,
	Bottom,
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

	~PersistentObjectList();
	void DeleteAll();
	void BeginAllocations();
	void EndAllocations();
	void DeleteRemaining();

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
		if (*_curPO) // a stable early-out branch that doesn't depend on inlining (TODO: use a separate bool w/o indirection?)
			DeleteRemaining();
		*_curPO = po;
		_curPO = &po->_next;
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

enum class EstSizeType
{
	Exact,
	Expanding,
};

struct EstSizeRange
{
	float softMin = 0;
	float hardMin = 0;
	float hardMax = FLT_MAX;

	EstSizeRange WithSoftMin(float osm) const
	{
		EstSizeRange r = *this;
		r.softMin = max(r.softMin, osm);
		return r;
	}
	EstSizeRange WithMins(EstSizeRange o) const
	{
		EstSizeRange r = *this;
		r.softMin = max(r.softMin, o.softMin);
		r.hardMin = max(r.hardMin, o.hardMin);
		return r;
	}
	EstSizeRange LimitTo(EstSizeRange o) const
	{
		EstSizeRange r = *this;
		r.softMin = clamp(max(r.softMin, o.softMin), o.hardMin, o.hardMax);
		r.hardMin = max(r.hardMin, o.hardMin);
		r.hardMax = min(r.hardMax, o.hardMax);
		return r;
	}
	EstSizeRange WithoutHardMax() const
	{
		return { softMin, hardMin };
	}
	EstSizeRange Add(float padding) const
	{
		EstSizeRange r = *this;
		r.softMin += padding;
		if (r.hardMin > 0)
			r.hardMin += padding;
		if (r.hardMax < FLT_MAX)
			r.hardMax += padding;
		return r;
	}
	EstSizeRange AddMins(EstSizeRange o) const
	{
		EstSizeRange r = *this;
		r.hardMin += o.hardMin;
		r.softMin += o.softMin;
		return r;
	}

	float ExpandToFill(float w) const
	{
		return max(hardMin, min(w, hardMax));
	}
	float GetMin() const
	{
		return max(hardMin, min(softMin, hardMax));
	}

	static EstSizeRange Exact(float size) { return { size, size, size }; }
	static EstSizeRange SoftExact(float size) { return { size, 0, size }; }
	static EstSizeRange SoftAtLeast(float size) { return { size }; }
};

struct LayoutInfo
{
	enum
	{
		FillH = 1 << 0,
		FillV = 1 << 1,
	};

	uint8_t flags;

	UI_FORCEINLINE LayoutInfo WithoutFillH() const { return { u8(flags & ~FillH) }; }
	UI_FORCEINLINE LayoutInfo WithoutFillV() const { return { u8(flags & ~FillV) }; }
	UI_FORCEINLINE LayoutInfo WithoutAnyFill() const { return { u8(flags & ~(FillH | FillV)) }; }
};

struct UIObject : IPersistentObject
{
	UIObject();
	// prevent accidental copies
	UIObject(const UIObject&) = delete;
	virtual ~UIObject();

	void PO_ResetConfiguration() override; // IPersistentObject
	void PO_BeforeDelete() override; // IPersistentObject
	void _InitReset();
	virtual void OnReset() {}

	virtual void OnEnable() {}
	virtual void OnDisable() {}
	virtual void OnEnterTree() {}
	virtual void OnExitTree() {}

	virtual void _AttachToFrameContents(FrameContents* owner);
	virtual void _DetachFromFrameContents();
	virtual void _DetachFromTree();

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

	virtual void OnPaint(const UIPaintContext& ctx) = 0;
	void Paint(const UIPaintContext& ctx);
	void RootPaint();
	virtual void OnPaintSingleChild(SingleChildPaintPtr* next, const UIPaintContext& ctx);

	void PerformLayout(const UIRect& rect, LayoutInfo info);
	void ApplyLayoutInfo(const UIRect& rectFromChild, const UIRect& layoutRect, LayoutInfo info);
	virtual EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) = 0;
	virtual Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) = 0;
	virtual void OnLayout(const UIRect& rect, LayoutInfo info) = 0;
	virtual void OnLayoutChanged() {}
	virtual void RedoLayout();

	virtual bool Contains(Point2f pos) const
	{
		return GetFinalRect().Contains(pos);
	}
	virtual Point2f LocalToChildPoint(Point2f pos) const { return pos; }
	virtual UIObject* FindObjectAtPoint(Point2f pos) = 0;

	void SetFlag(UIObjectFlags flag, bool set);
	static bool HasFlags(uint32_t total, UIObjectFlags f) { return (total & f) == f; }
	bool HasFlags(UIObjectFlags f) const { return (flags & f) == f; }
	bool InUse() const { return !!(flags & UIObject_IsClickedAnyMask) || IsFocused(); }
	bool _CanPaint() const { return !(flags & (UIObject_IsHidden | UIObject_NoPaint)); }
	bool _NeedsLayout() const { return !(flags & UIObject_IsHidden); }

	virtual void RemoveChildImpl(UIObject* ch) = 0;
	void DetachAll();
	void DetachParent();
	virtual void DetachChildren(bool recursive = false) = 0;
	virtual void AppendChild(UIObject* obj) = 0;

	bool IsChildOf(UIObject* obj) const;
	bool IsChildOrSame(UIObject* obj) const;
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

	LayoutInfo _rcvdLayoutInfo = {};
};

struct UIObjectIterator
{
	UIObject* _obj;
	UIObjectIteratorData _data;

	UI_FORCEINLINE UIObjectIterator(UIObject* o) : _obj(o)
	{
		o->SlotIterator_Init(_data);
	}
	UI_FORCEINLINE UIObject* GetNext()
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

struct UIObjectNoChildren : UIObject
{
	void SlotIterator_Init(UIObjectIteratorData& data) override {}
	UIObject* SlotIterator_GetNext(UIObjectIteratorData& data) override { return nullptr; }
	void RemoveChildImpl(UIObject* ch) override {}
	void DetachChildren(bool recursive) override {}
	void AppendChild(UIObject* obj) override;
	void OnPaint(const UIPaintContext& ctx) override {}
	UIObject* FindObjectAtPoint(Point2f pos) override { return Contains(pos) ? this : nullptr; }

	void OnLayout(const UIRect& rect, LayoutInfo info) override { _finalRect = rect; }
};

struct UIObjectSingleChild : UIObject
{
	UIObject* _child = nullptr;

	void OnReset() override;
	void SlotIterator_Init(UIObjectIteratorData& data) override;
	UIObject* SlotIterator_GetNext(UIObjectIteratorData& data) override;
	void RemoveChildImpl(UIObject* ch) override;
	void DetachChildren(bool recursive) override;
	void AppendChild(UIObject* obj) override;
	void OnPaint(const UIPaintContext& ctx) override;
	UIObject* FindObjectAtPoint(Point2f pos) override;
	void _AttachToFrameContents(FrameContents* owner) override;
	void _DetachFromFrameContents() override;
	void _DetachFromTree() override;
};

// TODO: slowly port to these
struct WrapperElement : UIObjectSingleChild
{
	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};

struct FillerElement : UIObjectSingleChild
{
	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};

struct HFillVWrapElement : UIObjectSingleChild
{
	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};

struct SizeConstraintElement : WrapperElement
{
	EstSizeRange widthRange;
	Rangef heightRange = Rangef::AtLeast(0);

	void OnReset() override;
	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;

	SizeConstraintElement& SetWidthRange(Rangef r) { widthRange = { r.min, r.min, r.max }; return *this; }
	SizeConstraintElement& SetHeightRange(Rangef r) { heightRange = r; return *this; }

	SizeConstraintElement& SetMinWidth(float w) { widthRange.hardMin = w; widthRange.softMin = w; return *this; }
	SizeConstraintElement& SetMaxWidth(float w) { widthRange.hardMax = w; return *this; }
	SizeConstraintElement& SetMinHeight(float h) { heightRange.min = h; return *this; }
	SizeConstraintElement& SetMaxHeight(float h) { heightRange.max = h; return *this; }

	SizeConstraintElement& SetWidth(float w) { widthRange = { w, w, w }; return *this; }
	SizeConstraintElement& SetHeight(float h) { heightRange.min = h; heightRange.max = h; return *this; }
	SizeConstraintElement& SetSize(float w, float h)
	{
		widthRange = { w, w, w };
		heightRange.min = h;
		heightRange.max = h;
		return *this;
	}
	SizeConstraintElement& SetSize(Size2f s)
	{
		widthRange = { s.x, s.x, s.x };
		heightRange.min = s.y;
		heightRange.max = s.y;
		return *this;
	}
};

struct CenteringElement : WrapperElement
{
	Vec2f align = { 0.5f, 0.5f };

	void OnReset() override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};

struct OverlayElement : WrapperElement
{
	float _depth = 0;
	bool _isRegistered = false;

	OverlayElement& Init(float depth) { return SetDepth(depth); }
	OverlayElement& SetDepth(float depth);

	void OnReset() override;
	void _AttachToFrameContents(FrameContents* owner) override;
	void _DetachFromFrameContents() override;
};

struct TextElement : UIObjectNoChildren
{
	std::string text;

	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;

	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;

	TextElement& SetText(StringView t)
	{
		text <<= t;
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
	UIObject* FindObjectAtPoint(Point2f pos) override;

	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};

struct DataCategoryTag {};

constexpr auto ANY_ITEM = uintptr_t(-1);

// a way to call operator new if it exists
namespace _ {
template<class T, class = void> struct has_operator_new : std::false_type {};
template<class T> struct has_operator_new<T, decltype(T::operator new)> : std::true_type {};

template <class T> UI_FORCEINLINE void* CallNew_DefSize(std::false_type) { return operator new(sizeof(T)); }
template <class T> UI_FORCEINLINE void* CallNew_DefSize(std::true_type) { return T::operator new(sizeof(T)); }
template <class T> UI_FORCEINLINE void* CallNew_DefSize() { return CallNew_DefSize<T>(has_operator_new<T>()); }
} // _

// intended to allow creating APIs for modifying global immediate mode UI state ..
// .. so that it travels through buildables correctly
// buildables are built out of order, which requires the global states ..
// .. to be copied into them and applied later when they're built
// note that a copy of these settings will be stored in *every* buildable ..
// .. therefore it is important to make these very efficient!
struct IGlobalIMUIStateSaver
{
	virtual void* CreateCurrentStateCopy() = 0;
	virtual void DestroyStateCopy(void* sc) = 0;
	virtual void ApplyStateCopy(void* sc) = 0;
};
// the saver is expected to remain available until the last builder is destroyed
void RegisterGlobalIMUIStateSaver(IGlobalIMUIStateSaver* saver);

struct Buildable : WrapperElement
{
	void PO_ResetConfiguration() override; // IPersistentObject
	void PO_BeforeDelete() override; // IPersistentObject

	virtual void Build() = 0;
	void Rebuild();

	void Defer(std::function<void()>&& fn)
	{
		_deferredDestructors.Append(Move(fn));
	}
	template <class T> void* NewT()
	{
		void* obj = _::CallNew_DefSize<T>();
		Defer([obj]() { delete static_cast<T*>(obj); });
		return obj;
	}

	PersistentObjectList _objList;
	uint64_t _lastBuildFrameID = 0;
	Array<std::function<void()>> _deferredDestructors;
	Array<void*> _globalStateCopies;
};


struct Modifier
{
	virtual void OnBeforeControl() const {}
	virtual void OnAfterControl() const {}
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

struct AddEventHandler : Modifier
{
	std::function<void(Event&)> _evfn;
	EventType _type = EventType::Any;
	UIObject* _tgt = nullptr;
	AddEventHandler(std::function<void(Event&)>&& fn) : _evfn(Move(fn)) {}
	AddEventHandler(EventType t, std::function<void(Event&)>&& fn) : _evfn(Move(fn)), _type(t) {}
	void Apply(UIObject* obj) const override { if (_evfn) obj->HandleEvent(_tgt, _type) = Move(_evfn); }
};

struct modAddTooltip : Modifier
{
	ui::Tooltip::BuildFunc _evfn;

	modAddTooltip(ui::Tooltip::BuildFunc&& fn) : _evfn(Move(fn)) {}
	modAddTooltip(StringView sv);
	void Apply(UIObject* obj) const override;
};

struct MakeDraggable : AddEventHandler
{
	MakeDraggable() : AddEventHandler({}) {}
	MakeDraggable(std::function<void(Event&)>&& fn) : AddEventHandler(EventType::DragStart, Move(fn)) {}
	MakeDraggable(std::function<void()>&& fn) : AddEventHandler(EventType::DragStart, [fn{ Move(fn) }](Event&){ fn(); }) {}
	void Apply(UIObject* obj) const override
	{
		AddEventHandler::Apply(obj);
		obj->SetFlag(UIObjectFlags::UIObject_DB_Draggable, true);
	}
};

namespace imm {
// utilities for immediate mode controls
struct imCtrlInfo
{
	bool edited;
	UIObject* root;

	UI_FORCEINLINE imCtrlInfo(bool ed, UIObject* o) : edited(ed), root(o) {}
	// void* to avoid unwanted implicit casting
	UI_FORCEINLINE operator void* () const { return (void*)edited; }

	UI_FORCEINLINE imCtrlInfo& Modify(const Modifier& mod)
	{
		mod.Apply(root);
		return *this;
	}
	UI_FORCEINLINE imCtrlInfo& Modify(ModInitList mods)
	{
		for (auto& mod : mods)
			mod->Apply(root);
		return *this;
	}
	UI_FORCEINLINE imCtrlInfo& Tooltip(StringView sv)
	{
		return Modify(modAddTooltip(sv));
	}
};
UI_FORCEINLINE bool operator | (bool v, const imCtrlInfo& ci) { return v | !!ci; }
UI_FORCEINLINE bool& operator |= (bool& v, const imCtrlInfo& ci) { return v |= !!ci; }
template <class UIObjT>
struct imCtrlInfoT : imCtrlInfo
{
	UI_FORCEINLINE imCtrlInfoT(bool ed, UIObjT* o) : imCtrlInfo(ed, o) {}
	UI_FORCEINLINE UIObjT& operator * () const { return *root; }
	UI_FORCEINLINE UIObjT* operator -> () const { return root; }
};
} // imm
using namespace imm;

} // ui
