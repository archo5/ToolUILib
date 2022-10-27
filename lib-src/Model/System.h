
#pragma once

#include <functional>

#include "../Core/Array.h"
#include "../Core/HashTable.h"
#include "Objects.h"
#include "EventSystem.h"


#define UI_DEBUG_FLOW(x) //x


namespace ui {

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

	void Append(UIObject* obj)
	{
		objectStack[objectStackSize - 1]->AppendChild(obj);
	}
	void Append(Buildable* obj)
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
	template<class T> T& Make()
	{
		T* obj = _Alloc<T>();
		UI_DEBUG_FLOW(printf("  make %s\n", typeid(*obj).name()));
		Append(obj);
		return *obj;
	}

	template<class T, class = typename NotBuildable<T>> T& MakeWithText(StringView text)
	{
		auto& ret = Push<T>();
		Text(text);
		Pop();
		return ret;
	}
	template<class T, class = typename NotBuildable<T>> T& MakeWithTextVA(const char* fmt, va_list args)
	{
		auto& ret = Push<T>();
		TextVA(fmt, args);
		Pop();
		return ret;
	}
	template<class T, class = typename NotBuildable<T>> T& MakeWithTextf(const char* fmt, ...)
	{
		auto& ret = Push<T>();
		va_list args;
		va_start(args, fmt);
		TextVA(fmt, args);
		va_end(args);
		Pop();
		return ret;
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
		Append(obj);
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

	TextElement& Text(StringView s)
	{
		auto& T = Make<TextElement>();
		T.SetText(s);
		return T;
	}
	TextElement& TextVA(const char* fmt, va_list args)
	{
		return Text(FormatVA(fmt, args));
	}
	TextElement& Textf(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		auto str = FormatVA(fmt, args);
		va_end(args);
		return Text(str);
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


void Pop();
template <class T> inline T& Make()
{
	return UIContainer::GetCurrent()->Make<T>();
}
template <class T, class = typename NotBuildable<T>> inline T& MakeWithText(StringView text)
{
	return UIContainer::GetCurrent()->MakeWithText<T>(text);
}
template <class T, class = typename NotBuildable<T>> inline T& MakeWithTextVA(const char* fmt, va_list args)
{
	return UIContainer::GetCurrent()->MakeWithTextVA<T>(fmt, args);
}
template <class T, class = typename NotBuildable<T>> inline T& MakeWithTextf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	auto& ret = UIContainer::GetCurrent()->MakeWithTextVA<T>(fmt, args);
	va_end(args);
	return ret;
}
template <class T, class = typename NotBuildable<T>> inline T& PushNoAppend()
{
	return UIContainer::GetCurrent()->PushNoAppend<T>();
}
template <class T, class = typename NotBuildable<T>> inline T& Push()
{
	return UIContainer::GetCurrent()->Push<T>();
}
inline void Append(UIObject* o)
{
	UIContainer::GetCurrent()->Append(o);
}
inline void Append(Buildable* o)
{
	UIContainer::GetCurrent()->Append(o);
}
TextElement& Text(StringView s);
TextElement& TextVA(const char* fmt, va_list args);
TextElement& Textf(const char* fmt, ...);
bool LastIsNew();

void RebuildCurrent();
Buildable* GetCurrentBuildable();
template <class T, class... Args> T* BuildAlloc(Args&&... args)
{
	return GetCurrentBuildable()->Allocate<T, Args...>(std::forward<Args>(args)...);
}
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
