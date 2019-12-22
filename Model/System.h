
#pragma once

#include <functional>

#include "Objects.h"


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

class UIObjectDirtyStack
{
public:

	UIObjectDirtyStack(uint32_t f) : flag(f) {}
	bool ContainsAny() const { return size != 0; }
	void ClearWithoutFlags()
	{
		size = 0;
	}
	void Add(UIObject* n);
	UIObject* Pop();
	void RemoveChildren();

	size_t debugpad1 = 0;
	UIObject* stack[128];
	size_t debugpad2 = 0;
	int size = 0;
	uint32_t flag;
};

class UIContainer
{
public:
	void Free();
	void ProcessObjectDeleteStack(int first = 0);
	void DeleteObjectsStartingFrom(UIObject* obj);
	template<class T> T* AllocIfDifferent(UIObject* obj)
	{
		if (obj && typeid(*obj) == typeid(T))
		{
			obj->Reset();
			return static_cast<T*>(obj);
		}
		auto* p = new T();
		p->system = owner;
		return p;
	}
	void AddToRenderStack(UINode* n)
	{
		nodeRenderStack.Add(n);
	}
	void ProcessNodeRenderStack();

	void _BuildUsing(UINode* n);

	void _Push(UIObject* obj);
	void _Destroy(UIObject* obj);
	void _Pop();
	void Pop()
	{
		assert(objectStackSize > 1);
		_Pop();
		printf("  pop [%d] %s\n", objectStackSize, typeid(*objectStack[objectStackSize]).name());
	}
	template<class T, class = typename T::IsNode> T* Make(decltype(bool()) = 0)
	{
		T* obj = _Alloc<T>();
		printf("  make %s\n", typeid(*obj).name());
		AddToRenderStack(obj);
		return obj;
	}
	template<class T, class = typename T::IsElement> T* Make(decltype(char()) = 0)
	{
		auto* ret = Push<T>();
		Pop();
		return ret;
	}
	template<class T, class = typename T::IsElement> T* MakeWithText(StringView text)
	{
		auto* ret = Push<T>();
		Text(text);
		Pop();
		return ret;
	}
	template<class T, class = typename T::IsElement> T* Push()
	{
		auto* obj = _Alloc<T>();
		printf("  push [%d] %s\n", objectStackSize, typeid(*obj).name());
		_Push(obj);
		return obj;
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
			puts("/// match streak ///");
			objChildStack[objectStackSize - 1] = obj->next;
			lastIsNew = false;
		}
		else
		{
			objectStack[objectStackSize - 1]->dump();
			if (objChildStack[objectStackSize - 1])
			{
				// delete all nodes starting from this child, the match streak is gone
				DeleteObjectsStartingFrom(objChildStack[objectStackSize - 1]);
			}
			puts(">>> new element >>>");
			Node_AddChild<UIObject>(objectStack[objectStackSize - 1], obj);
			obj->OnInit();
			objChildStack[objectStackSize - 1] = nullptr;
			lastIsNew = true;
		}
		return obj;
	}
	ui::BoxElement* PushBox() { return Push<ui::BoxElement>(); }
	ui::TextElement* Text(StringView s)
	{
		auto* T = Push<ui::TextElement>();
		T->SetText(s);
		Pop();
		return T;
	}
	bool LastIsNew() const { return lastIsNew; }

	ui::NativeWindowBase* GetNativeWindow() const;

	ui::FrameContents* owner = nullptr;
	UINode* rootNode = nullptr;
	int debugpad1 = 0;
	//UIElement* elementStack[128];
	//int debugpad2 = 0;
	UIObject* objectStack[128];
	int debugpad4 = 0;
	UIObject* objChildStack[128];
	//UINode* currentNode;
	//int elementStackSize = 0;
	int objectStackSize = 0;

	UIObjectDirtyStack nodeRenderStack{ UIObject_IsInRenderStack };

	bool isLayoutDirty = false;
	bool lastIsNew = false;
};

class UILayoutEngine
{
public:
};


namespace ui {

class RenderNode : public UINode
{
public:
	void Render(UIContainer* ctx) override
	{
		renderFunc(ctx);
	}

	std::function<void(UIContainer*)> renderFunc;
};

class FrameContents
{
public:
	FrameContents()
	{
		container.owner = this;
		eventSystem.container = &container;
	}

	UIContainer container;
	UIEventSystem eventSystem;
	ui::NativeWindowBase* nativeWindow;
};

class InlineFrameNode : public UINode
{
public:

	void OnDestroy() override;
	void OnEvent(UIEvent& ev) override;
	void OnPaint() override;
	float CalcEstimatedWidth(float containerWidth, float containerHeight) override;
	float CalcEstimatedHeight(float containerWidth, float containerHeight) override;
	void OnLayout(const UIRect& rect) override;
	void Render(UIContainer* ctx) override;

	void SetFrameContents(FrameContents* contents);
	void CreateFrameContents(std::function<void(UIContainer* ctx)> renderFunc);

private:

	FrameContents* frameContents = nullptr;
	bool ownsContents = false;
};

} // ui


inline const char* objtype(UIObject* obj)
{
	if (dynamic_cast<UINode*>(obj))
		return "node";
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
