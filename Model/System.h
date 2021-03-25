
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
		if (obj && typeid(*obj) == typeid(T))
		{
			auto* t = static_cast<T*>(obj);
			t->UnregisterAsOverlay();

			// TODO is there a way to optimize this?
			auto* buildable = dynamic_cast<Buildable*>(obj);
			decltype(buildable->_deferredDestructors) ddList;
			if (buildable)
				std::swap(buildable->_deferredDestructors, ddList);

			if (T::Persistent)
			{
				t->_Reset();
			}
			else
			{
				char buf[1024];
				DataWriteSerializer dws(buf);
				t->_SerializePersistent(dws);
				t->~T();
				new (t) T();
				auto origFlags = t->flags;
				DataReadSerializer drs(buf);
				t->_SerializePersistent(drs);
				// in case these flags have been set by ctor
				t->flags |= origFlags & (UIObject_IsInLayoutStack | UIObject_IsInBuildStack);
			}

			if (buildable)
				std::swap(buildable->_deferredDestructors, ddList);

			t->_OnChangeStyle();
			return t;
		}
		auto* p = new T();
		p->system = owner;
		p->_OnChangeStyle();
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
		if (obj == objChildStack[objectStackSize - 1])
		{
			// continue the match streak
			UI_DEBUG_FLOW(puts("/// match streak ///"));
			objChildStack[objectStackSize - 1] = obj->next;
			lastIsNew = false;
		}
		else
		{
			UI_DEBUG_FLOW(objectStack[objectStackSize - 1]->dump());
			if (objChildStack[objectStackSize - 1])
			{
				// delete all buildables starting from this child, the match streak is gone
				DeleteObjectsStartingFrom(objChildStack[objectStackSize - 1]);
			}
			UI_DEBUG_FLOW(puts(">>> new element >>>"));
			Node_AddChild<UIObject>(objectStack[objectStackSize - 1], obj);
			objChildStack[objectStackSize - 1] = nullptr;
			lastIsNew = true;
		}
		obj->OnInit();
		_lastCreated = obj;
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

	ui::NativeWindowBase* GetNativeWindow() const;
	Buildable* GetCurrentBuildable() const { return _curBuildable; }

	ui::FrameContents* owner = nullptr;
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
};


class BuildCallback : public Buildable
{
public:
	void Build(UIContainer* ctx) override
	{
		buildFunc(ctx);
	}

	std::function<void(UIContainer*)> buildFunc;
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

typedef Buildable* BuildableAllocFunc(UIContainer*);
template <class T> inline Buildable* BuildableAlloc(UIContainer* ctx)
{
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
	float CalcEstimatedWidth(const Size2f& containerSize, style::EstSizeType type) override;
	float CalcEstimatedHeight(const Size2f& containerSize, style::EstSizeType type) override;
	void OnLayoutChanged() override;
	void Build(UIContainer* ctx) override;

	void SetFrameContents(FrameContents* contents);
	void CreateFrameContents(std::function<void(UIContainer* ctx)> buildFunc);

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
