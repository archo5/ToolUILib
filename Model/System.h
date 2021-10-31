
#pragma once

#include <functional>

#include "../Core/HashTable.h"
#include "Objects.h"


#define UI_DEBUG_FLOW(x) //x


namespace ui {

template<class T> void Node_AddChild(T* node, T* ch)
{
	assert(ch->parent == nullptr);
	ch->parent = node;
	if (!node->firstChild)
	{
		node->firstChild = node->lastChild = ch;
	}
	else
	{
		ch->prev = node->lastChild;
		node->lastChild->next = ch;
		node->lastChild = ch;
	}
}

struct UIObjectDirtyStack
{
	UIObjectDirtyStack(uint32_t f) : flag(f) {}
	bool ContainsAny() const { return stack.size() != 0; }
	void ClearWithoutFlags()
	{
		stack.clear();
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
	size_t Size() const { return stack.size(); }
	void RemoveNth(size_t n);

	std::vector<UIObject*> stack;
	uint32_t flag;
};

struct UIContainer
{
	void Free();
	void ProcessObjectDeleteStack(int first = 0);
	void DeleteObjectsStartingFrom(UIObject* obj);
	template<class T> T* AllocIfDifferent(UIObject* obj)
	{
		if (obj && typeid(*obj) == typeid(T) && (obj->flags & UIObject_BuildAlloc))
		{
			obj->PO_ResetConfiguration();
			return static_cast<T*>(obj);
		}
		auto* p = new T();
		p->flags |= UIObject_BuildAlloc;
		p->_OnChangeStyle();
		p->PO_ResetConfiguration();
		return p;
	}
	void AddToBuildStack(Buildable* n)
	{
		UI_DEBUG_FLOW(printf("add %p to build stack\n", n));
		if (n->_lastBuildFrameID != _lastBuildFrameID)
			buildStack.Add(n);
		else
			nextFrameBuildStack.Add(n);
	}
	void ProcessBuildStack();
	void ProcessLayoutStack();

	void _BuildUsing(Buildable* n);

	void _Push(UIObject* obj, bool isCurBuildable);
	void _Destroy(UIObject* obj);
	void _Pop();
	void _AllocReplace(UIObject* obj);
	void Append(UIObject* o);
	void Pop()
	{
		assert(objectStackSize > 1);
		_Pop();
		UI_DEBUG_FLOW(printf("  pop [%d] %s\n", objectStackSize, typeid(*objectStack[objectStackSize]).name()));
	}
	template<class T, class = typename T::IsBuildable> T& Make(decltype(bool()) = 0)
	{
		T* obj = _Alloc<T>();
		obj->_lastBuildFrameID = _lastBuildFrameID - 1;
		UI_DEBUG_FLOW(printf("  make %s\n", typeid(*obj).name()));
		AddToBuildStack(obj);
		return *obj;
	}
	template<class T, class = typename T::IsElement> T& Make(decltype(char()) = 0)
	{
		auto& ret = Push<T>();
		Pop();
		return ret;
	}

	template<class T, class = typename T::IsElement> T& MakeWithText(StringView text)
	{
		auto& ret = Push<T>();
		Text(text);
		Pop();
		return ret;
	}
	template<class T, class = typename T::IsElement> T& MakeWithTextVA(const char* fmt, va_list args)
	{
		auto& ret = Push<T>();
		TextVA(fmt, args);
		Pop();
		return ret;
	}
	template<class T, class = typename T::IsElement> T& MakeWithTextf(const char* fmt, ...)
	{
		auto& ret = Push<T>();
		va_list args;
		va_start(args, fmt);
		TextVA(fmt, args);
		va_end(args);
		Pop();
		return ret;
	}

	template<class T, class = typename T::IsElement> T& Push()
	{
		auto* obj = _Alloc<T>();
		UI_DEBUG_FLOW(printf("  push [%d] %s\n", objectStackSize, typeid(*obj).name()));
		_Push(obj, false);
		return *obj;
	}
	template<class T> T* _Alloc()
	{
		//printf("%p\n", objChildStack[objectStackSize - 1]);
		//if (objChildStack[objectStackSize - 1])
		//	printf("%s\n", typeid(*objChildStack[objectStackSize - 1]).name());
		T* obj = AllocIfDifferent<T>(objChildStack[objectStackSize - 1]);
		_AllocReplace(obj);
		return obj;
	}
	BoxElement& PushBox() { return Push<BoxElement>(); }

	TextElement& Text(StringView s)
	{
		auto& T = Push<TextElement>();
		T.SetText(s);
		Pop();
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
	UIObject* GetLastCreated() const { return _lastCreated; }

	NativeWindowBase* GetNativeWindow() const;
	Buildable* GetCurrentBuildable() const { return _curBuildable; }

	FrameContents* owner = nullptr;
	Buildable* rootBuildable = nullptr;
	Buildable* _curBuildable = nullptr;
	UIObject* _lastCreated = nullptr;
	int debugpad1 = 0;
	//UIElement* elementStack[128];
	//int debugpad2 = 0;
	UIObject* objectStack[1024];
	int debugpad4 = 0;
	UIObject* objChildStack[1024];
	//Buildable* currentBuildable;
	//int elementStackSize = 0;
	int objectStackSize = 0;
	uint64_t _lastBuildFrameID = 1;

	UIObjectDirtyStack buildStack{ UIObject_IsInBuildStack };
	UIObjectDirtyStack nextFrameBuildStack{ UIObject_IsInBuildStack };
	UIObjectDirtyStack layoutStack{ UIObject_IsInLayoutStack };

	bool lastIsNew = false;

	static UIContainer* GetCurrent();
};


void Pop();
template <class T, class = typename T::IsBuildable> inline T& Make(decltype(bool()) = 0)
{
	return UIContainer::GetCurrent()->Make<T>();
}
template <class T, class = typename T::IsElement> inline T& Make(decltype(char()) = 0)
{
	return UIContainer::GetCurrent()->Make<T>();
}
template <class T, class = typename T::IsElement> inline T& MakeWithText(StringView text)
{
	return UIContainer::GetCurrent()->MakeWithText<T>(text);
}
template <class T, class = typename T::IsElement> inline T& MakeWithTextVA(const char* fmt, va_list args)
{
	return UIContainer::GetCurrent()->MakeWithTextVA<T>(fmt, args);
}
template <class T, class = typename T::IsElement> inline T& MakeWithTextf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	auto& ret = UIContainer::GetCurrent()->MakeWithTextVA<T>(fmt, args);
	va_end(args);
	return ret;
}
template <class T, class = typename T::IsElement> inline T& Push()
{
	return UIContainer::GetCurrent()->Push<T>();
}
inline void Append(UIObject* o)
{
	UIContainer::GetCurrent()->Append(o);
}
BoxElement& PushBox();
TextElement& Text(StringView s);
TextElement& TextVA(const char* fmt, va_list args);
TextElement& Textf(const char* fmt, ...);
template <class T, class... Args> T* BuildAlloc(Args&&... args)
{
	return UIContainer::GetCurrent()->GetCurrentBuildable()->Allocate<T, Args...>(std::forward<Args>(args)...);
}
void RebuildCurrent();
Buildable* GetCurrentBuildable();
bool LastIsNew();


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
	std::vector<Sorted> sorted;
	bool sortedOutdated = false;
};

typedef Buildable* BuildableAllocFunc();
template <class T> inline Buildable* BuildableAlloc()
{
	auto* ctx = UIContainer::GetCurrent();
	return ctx->AllocIfDifferent<T>(ctx->rootBuildable);
}

class InlineFrame;
struct FrameContents
{
	FrameContents();
	~FrameContents();
	template <class T> T* AllocRoot()
	{
		return static_cast<T*>(_AllocRootImpl(BuildableAlloc<T>));
	}
	Buildable* _AllocRootImpl(BuildableAllocFunc* f);
	void BuildRoot();

	UIContainer container;
	EventSystem eventSystem;
	Overlays overlays;
	NativeWindowBase* nativeWindow = nullptr;
	InlineFrame* owningFrame = nullptr;
};

class InlineFrame : public Buildable
{
public:

	void OnDestroy() override;
	void OnEvent(Event& ev) override;
	void OnPaint() override;
	float CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	float CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayoutChanged() override;
	void Build() override;

	void SetFrameContents(FrameContents* contents);
	void CreateFrameContents(std::function<void()> buildFunc);

private:

	FrameContents* frameContents = nullptr;
	bool ownsContents = false;
};


inline const char* objtype(UIObject* obj)
{
	if (dynamic_cast<Buildable*>(obj))
		return "buildable";
	if (dynamic_cast<UIElement*>(obj))
		return "element";
	return "object";
}

inline void dumptree(UIObject* obj, int lev = 0)
{
	if (lev == 0) puts("");
	for (int i = 0; i < lev; i++)
		printf("  ");
	printf("%s - %s\n", typeid(*obj).name(), objtype(obj));
	for (UIObject* ch = obj->firstChild; ch; ch = ch->next)
	{
		dumptree(ch, lev + 1);
	}
	if (lev == 0) puts("");
}

} // ui
