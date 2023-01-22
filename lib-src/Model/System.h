
#pragma once

#include <functional>

#include "../Core/Array.h"
#include "../Core/HashMap.h"
#include "../Core/Logging.h"
#include "Objects.h"
#include "EventSystem.h"


#define UI_DEBUG_FLOW(x) //x


namespace ui {

extern LogCategory LOG_UISYS;

struct UIObjectDirtyStack
{
	UIObjectDirtyStack(uint32_t f) : flag(f) {}
	bool ContainsAny() const { return stack.NotEmpty(); }
	void ClearWithoutFlags()
	{
		stack.Clear();
	}
	void Clear();
	void Swap(UIObjectDirtyStack& o)
	{
		assert(flag == o.flag);
		std::swap(stack, o.stack);
	}
	void Add(UIObject* n);
	void OnDestroy(UIObject* n);
	UIObject* Pop();
	void RemoveChildren();
	size_t Size() const { return stack.Size(); }
	void RemoveNth(size_t n);

	Array<UIObject*> stack;
	uint32_t flag;
};

struct UIObjectPendingDeactivationSet
{
	Array<UIObject*> _data;

	void InsertIfMissing(UIObject* obj)
	{
		if (obj->_pendingDeactivationSetPos != UINT32_MAX)
			return;
		Insert(obj);
	}
	void Insert(UIObject* obj)
	{
		obj->_pendingDeactivationSetPos = _data.Size();
		_data.Append(obj);
	}
	void RemoveIfFound(UIObject* obj)
	{
		if (obj->_pendingDeactivationSetPos == UINT32_MAX)
			return;
		Remove(obj);
	}
	void Remove(UIObject* obj)
	{
		auto pos = obj->_pendingDeactivationSetPos;
		obj->_pendingDeactivationSetPos = UINT32_MAX;

		if (pos + 1 < _data.Size())
		{
			_data[pos] = _data.Last();
			_data[pos]->_pendingDeactivationSetPos = pos;
		}
		_data.RemoveLast();
	}
	void Flush()
	{
		while (_data.NotEmpty())
		{
			UIObject* obj = _data.Last();
			obj->_DetachFromFrameContents();
		}
		assert(_data.IsEmpty() && "some object continues to add itself to the pending deactivation set!");
	}
};

namespace imm
{
bool GetEnabled();
bool SetEnabled(bool);
}

template <class T>
using NotBuildable = std::enable_if_t<!std::is_base_of<Buildable, T>::value>;

struct UIContainer
{
	void Free();

	void AddToBuildStack(Buildable* n)
	{
		if (!(n->flags & UIObject_IsInTree))
			return;
		UI_DEBUG_FLOW(printf("add %p to build stack\n", n));
		if (n->_lastBuildFrameID != _lastBuildFrameID)
			buildStack.Add(n);
		else
			nextFrameBuildStack.Add(n);
	}
	void ProcessBuildStack();
	void ProcessLayoutStack();

	void _BuildUsing(Buildable* n);

	void Add(UIObject* obj)
	{
		objectStack[objectStackSize - 1]->AppendChild(obj);
	}
	void Add(Buildable* obj)
	{
		objectStack[objectStackSize - 1]->AppendChild(obj);
		if (!imm::GetEnabled())
			obj->flags |= UIObject_IsDisabled;
		obj->_lastBuildFrameID = _lastBuildFrameID - 1;
		AddToBuildStack(obj);
	}
	void _Push(UIObject* obj);
	void _Pop();
	void Pop()
	{
		assert(objectStackSize > 1);
		_Pop();
		UI_DEBUG_FLOW(printf("  pop [%d] %s\n", objectStackSize, typeid(*objectStack[objectStackSize]).name()));
	}
	template<class T> T& AddNew()
	{
		T* obj = _Alloc<T>();
		UI_DEBUG_FLOW(printf("  make %s\n", typeid(*obj).name()));
		Add(obj);
		return *obj;
	}

	template<class T, class = typename NotBuildable<T>> T& PushNoAppend()
	{
		auto* obj = _Alloc<T>();
		UI_DEBUG_FLOW(printf("  push-no-append [%d] %s\n", objectStackSize, typeid(*obj).name()));
		_Push(obj);
		return *obj;
	}
	template<class T, class = typename NotBuildable<T>> T& Push()
	{
		auto* obj = _Alloc<T>();
		UI_DEBUG_FLOW(printf("  push [%d] %s\n", objectStackSize, typeid(*obj).name()));
		Add(obj);
		_Push(obj);
		return *obj;
	}
	template<class T> T* _Alloc()
	{
		T* obj = _curObjectList->TryNext<T>();
		lastIsNew = obj == nullptr;
		if (!obj)
		{
			obj = CreateUIObject<T>();
			_curObjectList->AddNext(obj);
		}
		return obj;
	}

	bool LastIsNew() const { return lastIsNew; }

	NativeWindowBase* GetNativeWindow() const;
	Buildable* GetCurrentBuildable() const { return _curBuildable; }

	FrameContents* owner = nullptr;
	Buildable* rootBuildable = nullptr;
	Buildable* _curBuildable = nullptr;
	PersistentObjectList* _curObjectList = nullptr;
	int debugpad1 = 0;
	UIObject* objectStack[1024];
	int debugpad4 = 0;
	int objectStackSize = 0;
	uint64_t _lastBuildFrameID = 1;

	UIObjectDirtyStack buildStack{ UIObject_IsInBuildStack };
	UIObjectDirtyStack nextFrameBuildStack{ UIObject_IsInBuildStack };
	UIObjectDirtyStack layoutStack{ UIObject_IsInLayoutStack };
	UIObjectPendingDeactivationSet pendingDeactivationSet;

	bool lastIsNew = false;

	static UIContainer* GetCurrent();
};


namespace _ {
extern UIContainer* g_curContainer; // do not use directly, this is for optimization purposes only
} // _

void Pop();
template <class T> inline T& New()
{
	return *_::g_curContainer->_Alloc<T>();
}
template <class T> inline T& Wrap(UIObject& obj)
{
	auto& p = New<T>();
	p.AppendChild(&obj);
	return p;
}
template <class T> inline T& Make() // deprecated
{
	return _::g_curContainer->AddNew<T>();
}
template <class T> inline T& AddNew()
{
	return _::g_curContainer->AddNew<T>();
}
template <class T> inline T& AddWrappedIn(UIObject& obj)
{
	auto& p = AddNew<T>();
	p.AppendChild(&obj);
	return p;
}
template <class T, class = typename NotBuildable<T>> inline T& PushNoAppend()
{
	return _::g_curContainer->PushNoAppend<T>();
}
template <class T, class = typename NotBuildable<T>> inline T& Push()
{
	return _::g_curContainer->Push<T>();
}
inline void Add(UIObject* o)
{
	_::g_curContainer->Add(o);
}
inline void Add(Buildable* o)
{
	_::g_curContainer->Add(o);
}
inline void Add(UIObject& o)
{
	_::g_curContainer->Add(&o);
}
inline void Add(Buildable& o)
{
	_::g_curContainer->Add(&o);
}
TextElement& NewText(StringView s);
TextElement& NewTextVA(const char* fmt, va_list args);
TextElement& NewTextf(const char* fmt, ...);
TextElement& Text(StringView s);
TextElement& TextVA(const char* fmt, va_list args);
TextElement& Textf(const char* fmt, ...);

template <class T, class = typename NotBuildable<T>> inline T& MakeWithText(StringView text)
{
	return AddWrappedIn<T>(NewText(text));
}
template <class T, class = typename NotBuildable<T>> inline T& MakeWithTextVA(const char* fmt, va_list args)
{
	return AddWrappedIn<T>(NewTextVA(fmt, args));
}
template <class T, class = typename NotBuildable<T>> inline T& MakeWithTextf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	auto& ret = AddWrappedIn<T>(NewTextVA(fmt, args));
	va_end(args);
	return ret;
}

bool LastIsNew();

void RebuildCurrent();
Buildable* GetCurrentBuildable();
template <class T, class... Args> T* BuildAlloc(Args&&... args)
{
	return GetCurrentBuildable()->Allocate<T, Args...>(std::forward<Args>(args)...);
}
#define UI_BUILD_ALLOC(T) new (::ui::GetCurrentBuildable()->NewT<T>()) T
template <class F> void BuildDefer(F&& f) { GetCurrentBuildable()->Defer(std::move(f)); }
template <class MD, class F> void BuildMulticastDelegateAdd(MD& md, F&& f)
{
	auto* e = md.Add(std::move(f));
	BuildDefer([e]() { e->Destroy(); });
}
template <class MD, class F> void BuildMulticastDelegateAddNoArgs(MD& md, F&& f)
{
	auto* e = md.AddNoArgs(std::move(f));
	BuildDefer([e]() { e->Destroy(); });
}


class BuildCallback : public Buildable
{
public:
	void Build() override
	{
		buildFunc();
	}

	std::function<void()> buildFunc;
};

struct Overlays
{
	struct Info
	{
		float depth;
	};
	struct Sorted
	{
		UIObject* obj;
		float depth;
	};

	void Register(UIObject* obj, float depth);
	void Unregister(UIObject* obj);
	void UpdateSorted();

	// TODO hide impl?
	HashMap<UIObject*, Info> mapped;
	Array<Sorted> sorted;
	bool sortedOutdated = false;
};

struct InlineFrame;
struct FrameContents
{
	FrameContents();
	~FrameContents();
	void BuildRoot(Buildable* B);

	UIContainer container;
	EventSystem eventSystem;
	Overlays overlays;
	NativeWindowBase* nativeWindow = nullptr;
	WeakPtr<InlineFrame> owningFrame = nullptr;
};

struct InlineFrame : Buildable
{
	FrameContents* _frameContents = nullptr;
	bool _ownsContents = false;

	void OnReset() override;
	void OnEvent(Event& ev) override;
	void OnPaint(const UIPaintContext& ctx) override;
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayoutChanged() override;
	void Build() override;

	void SetFrameContents(FrameContents* contents);
	void CreateFrameContents(std::function<void()> buildFunc);
};

} // ui
