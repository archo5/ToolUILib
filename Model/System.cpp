
#include "System.h"

#include <algorithm>


namespace ui {

extern uint32_t g_curLayoutFrame;
FrameContents* g_curSystem;
UIContainer* g_curContainer;


#if 0
#define CHECK_NOT_IN_DIRTY_STACK_SINCE(n, start) for (size_t __i = start; __i < stack.size(); __i++) if (stack[__i] == n) __debugbreak();
#else
#define CHECK_NOT_IN_DIRTY_STACK_SINCE(n, start)
#endif
#define CHECK_NOT_IN_DIRTY_STACK(n) CHECK_NOT_IN_DIRTY_STACK_SINCE(n, 0)

void UIObjectDirtyStack::Clear()
{
	for (auto* obj : stack)
		obj->flags &= ~flag;
	stack.clear();
}

void UIObjectDirtyStack::Add(UIObject* n)
{
#if 0
	{
		int count = 0;
		for (auto* o : stack)
			if (o == n)
				count++;
		if (count && !(n->flags & flag))
			__debugbreak();
		printf("INSERTED %p already %d times, flag=%d\n", n, count, n->flags & flag);
	}
#endif
	if (n->flags & flag)
		return;
	stack.push_back(n);
	n->flags |= flag;
}

void UIObjectDirtyStack::OnDestroy(UIObject* n)
{
	if (!(n->flags & flag))
	{
		CHECK_NOT_IN_DIRTY_STACK(n);
		return;
	}
	for (auto it = stack.begin(); it != stack.end(); it++)
	{
		if (*it == n)
		{
			if (it + 1 != stack.end())
				*it = stack.back();
			stack.pop_back();
			break;
		}
	}
	CHECK_NOT_IN_DIRTY_STACK(n);
}

UIObject* UIObjectDirtyStack::Pop()
{
	assert(stack.size() > 0);
	UIObject* n = stack.back();
	stack.pop_back();
	n->flags &= ~flag;
	CHECK_NOT_IN_DIRTY_STACK(n);
	return n;
}

void UIObjectDirtyStack::RemoveChildren()
{
	for (size_t i = 0; i < stack.size(); i++)
	{
		CHECK_NOT_IN_DIRTY_STACK_SINCE(stack[i], i + 1);
		auto* p = stack[i]->parent;
		while (p)
		{
			if (p->flags & flag)
				break;
			p = p->parent;
		}
		if (p)
			RemoveNth(i--);
	}
}

void UIObjectDirtyStack::RemoveNth(size_t n)
{
	auto* src = stack[n];

	src->flags &= ~flag;
	if (n + 1 < stack.size())
		stack[n] = stack.back();
	stack.pop_back();

	CHECK_NOT_IN_DIRTY_STACK(src);
}


void UIContainer::Free()
{
	if (rootBuildable)
	{
		_Destroy(rootBuildable);
		objectStack[0] = rootBuildable;
		objectStackSize = 1;
		ProcessObjectDeleteStack();
		rootBuildable = nullptr;
	}
}

void UIContainer::ProcessObjectDeleteStack(int first)
{
	while (objectStackSize > first)
	{
		auto* cur = objectStack[--objectStackSize];

		for (auto* n = cur->firstChild; n != nullptr; n = n->next)
			objectStack[objectStackSize++] = n;

		UI_DEBUG_FLOW(printf("    deleting %p\n", cur));
		owner->eventSystem.OnDestroy(cur);
		buildStack.OnDestroy(cur);
		nextFrameBuildStack.OnDestroy(cur);
		layoutStack.OnDestroy(cur);
		if (cur->flags & UIObject_BuildAlloc)
			delete cur;
		else
			cur->parent = nullptr;
	}
}

void UIContainer::DeleteObjectsStartingFrom(UIObject* obj)
{
	_Destroy(obj);

	obj->parent->lastChild = obj->prev;
	if (obj->prev)
		obj->prev->next = nullptr;
	else
		obj->parent->firstChild = nullptr;

	int first = objectStackSize;
	for (auto* o = obj; o; o = o->next)
		objectStack[objectStackSize++] = o;
	ProcessObjectDeleteStack(first);
}

void UIContainer::ProcessBuildStack()
{
	if (buildStack.ContainsAny())
	{
		UI_DEBUG_FLOW(puts(" ---- processing node BUILD stack ----"));
		_lastBuildFrameID++;
	}
	else
		return;

	TmpEdit<decltype(g_curSystem)> tmp(g_curSystem, owner);
	TmpEdit<decltype(g_curContainer)> tmp2(g_curContainer, this);

	buildStack.RemoveChildren();

	while (buildStack.ContainsAny())
	{
		Buildable* currentBuildable = static_cast<Buildable*>(buildStack.Pop());

		objectStackSize = 0;
		_Push(currentBuildable, true);

		UI_DEBUG_FLOW(printf("building %s\n", typeid(*currentBuildable).name()));
		_curBuildable = currentBuildable;
		currentBuildable->_lastBuildFrameID = _lastBuildFrameID;

		currentBuildable->ClearLocalEventHandlers();

		// do not run old dtors before build (so that mid-build all data is still valid)
		// but have the space cleaned out for the new dtors
		decltype(Buildable::_deferredDestructors) oldDDs;
		std::swap(oldDDs, currentBuildable->_deferredDestructors);

		currentBuildable->Build();

		while (oldDDs.size())
		{
			oldDDs.back()();
			oldDDs.pop_back();
		}

		_curBuildable = nullptr;

		if (objectStackSize > 1)
		{
			printf("WARNING: elements not popped: %d\n", objectStackSize);
			while (objectStackSize > 1)
			{
				Pop();
			}
		}

		_Pop(); // root
	}

	buildStack.Swap(nextFrameBuildStack);
	_lastBuildFrameID++;
}

void UIContainer::ProcessLayoutStack()
{
	if (layoutStack.ContainsAny())
	{
		UI_DEBUG_FLOW(printf(" ---- processing node LAYOUT stack (%zu items) ----\n", layoutStack.stack.size()));
	}
	else
		return;

	TmpEdit<decltype(g_curSystem)> tmp(g_curSystem, owner);

	// TODO check if the styles are actually different and if not, remove element from the stack

	layoutStack.RemoveChildren();

	// TODO change elements to their parents if parent/self layout combo indicates that parent will be affected
	for (size_t i = 0; i < layoutStack.stack.size(); i++)
	{
		auto* obj = layoutStack.stack[i];
		auto* p = obj;
		while (p->parent)
			p = p->parent;
		if (p != obj)
		{
			layoutStack.Add(p);
			layoutStack.RemoveNth(i--);
		}
	}

	layoutStack.RemoveChildren();

	assert(!layoutStack.stack.empty());
	g_curLayoutFrame++;

	// single pass
	for (UIObject* obj : layoutStack.stack)
	{
		// TODO how to restart layout?
		printf("relayout %s @ %p\n", typeid(*obj).name(), obj);
		obj->OnLayout(obj->lastLayoutInputRect, obj->lastLayoutInputCSize);
		obj->OnLayoutChanged();
	}
	layoutStack.Clear();
}

void UIContainer::_BuildUsing(Buildable* n)
{
	n->_SetOwner(owner);
	rootBuildable = n;
	assert(!buildStack.ContainsAny());
	buildStack.ClearWithoutFlags(); // TODO: is this needed?
	AddToBuildStack(rootBuildable);
	ProcessBuildStack();
	ProcessLayoutStack();
}

void UIContainer::_Push(UIObject* obj, bool isCurBuildable)
{
	objectStack[objectStackSize] = obj;
	objChildStack[objectStackSize] = obj->firstChild;
	objectStackSize++;
}

void UIContainer::_Destroy(UIObject* obj)
{
	UI_DEBUG_FLOW(printf("    destroy %p\n", obj));
	obj->_livenessToken.SetAlive(false);
	obj->_UnsetOwner();
	for (auto* ch = obj->firstChild; ch; ch = ch->next)
		_Destroy(ch);
}

void UIContainer::_Pop()
{
	_lastCreated = objectStack[objectStackSize - 1];
	if (objChildStack[objectStackSize - 1])
	{
		// remove leftover children
		DeleteObjectsStartingFrom(objChildStack[objectStackSize - 1]);
	}

	objectStack[objectStackSize - 1]->_livenessToken.SetAlive(true);

	objectStackSize--;
}

void UIContainer::_AllocReplace(UIObject* obj)
{
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
	obj->_SetOwner(owner);
	_lastCreated = obj;
}

void UIContainer::Append(UIObject* o)
{
	_AllocReplace(o);
	// TODO needed?
	_Push(o, false);
	_Pop();
}

NativeWindowBase* UIContainer::GetNativeWindow() const
{
	return owner->nativeWindow;
}

UIContainer* UIContainer::GetCurrent()
{
	return g_curContainer;
}


void Pop()
{
	return g_curContainer->Pop();
}

BoxElement& PushBox()
{
	return g_curContainer->PushBox();
}

TextElement& Text(StringView s)
{
	return g_curContainer->Text(s);
}

TextElement& TextVA(const char* fmt, va_list args)
{
	return g_curContainer->TextVA(fmt, args);
}

TextElement& Textf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	auto str = FormatVA(fmt, args);
	va_end(args);
	return Text(str);
}

void RebuildCurrent()
{
	GetCurrentBuildable()->Rebuild();
}

Buildable* GetCurrentBuildable()
{
	return UIContainer::GetCurrent()->GetCurrentBuildable();
}

bool LastIsNew()
{
	return UIContainer::GetCurrent()->LastIsNew();
}


void Overlays::Register(UIObject* obj, float depth)
{
	mapped[obj] = { depth };
	sortedOutdated = true;
}

void Overlays::Unregister(UIObject* obj)
{
	mapped.erase(obj);
	sortedOutdated = true;
}

void Overlays::UpdateSorted()
{
	if (!sortedOutdated)
		return;

	sorted.clear();
	sorted.reserve(mapped.size());
	for (const auto& kvp : mapped)
		sorted.push_back({ kvp.key, kvp.value.depth });
	std::sort(sorted.begin(), sorted.end(), [](const Sorted& a, const Sorted& b) { return a.depth < b.depth; });
	sortedOutdated = false;
}


FrameContents::FrameContents()
{
	container.owner = this;
	eventSystem.container = &container;
	eventSystem.overlays = &overlays;
}

FrameContents::~FrameContents()
{
	container.Free();
}

Buildable* FrameContents::_AllocRootImpl(BuildableAllocFunc* f)
{
	TmpEdit<decltype(g_curSystem)> tmp(g_curSystem, this);
	TmpEdit<decltype(g_curContainer)> tmp2(g_curContainer, &container);
	auto* N = f();
	container.rootBuildable = N;
	return N;
}

void FrameContents::BuildRoot()
{
	TmpEdit<decltype(g_curSystem)> tmp(g_curSystem, this);
	TmpEdit<decltype(g_curContainer)> tmp2(g_curContainer, &container);
	container._BuildUsing(container.rootBuildable);
}


void InlineFrame::OnDestroy()
{
	if (ownsContents && frameContents)
	{
		delete frameContents;
		frameContents = nullptr;
		ownsContents = false;
	}
}

void InlineFrame::Build()
{
}

void InlineFrame::OnEvent(Event& ev)
{
	if (frameContents)
	{
		// pass events
		if (ev.type == EventType::ButtonDown || ev.type == EventType::ButtonUp)
			frameContents->eventSystem.OnMouseButton(ev.type == EventType::ButtonDown, ev.GetButton(), ev.position, ev.modifiers);
		if (ev.type == EventType::MouseMove)
			frameContents->eventSystem.OnMouseMove(ev.position, ev.modifiers);
	}
	Buildable::OnEvent(ev);
}

void InlineFrame::OnPaint()
{
	styleProps->background_painter->Paint(this);

	if (frameContents &&
		frameContents->container.rootBuildable)
		frameContents->container.rootBuildable->OnPaint();

	PaintChildren();
}

float InlineFrame::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	return 100; // default width
}

float InlineFrame::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	return 100; // default height
}

void InlineFrame::OnLayoutChanged()
{
	if (frameContents &&
		frameContents->container.rootBuildable)
		frameContents->container.rootBuildable->PerformLayout(finalRectC, finalRectC.GetSize());
}

void InlineFrame::SetFrameContents(FrameContents* contents)
{
	if (frameContents)
	{
		if (ownsContents)
		{
			delete frameContents;
		}
		else
		{
			frameContents->owningFrame = nullptr;
			frameContents->nativeWindow = nullptr;
		}
	}
	if (contents)
	{
		if (contents->owningFrame)
		{
			contents->owningFrame->frameContents = nullptr;
		}
		contents->nativeWindow = GetNativeWindow();
		contents->owningFrame = this;
	}
	frameContents = contents;
	ownsContents = false;
	_OnChangeStyle();
}

void InlineFrame::CreateFrameContents(std::function<void()> buildFunc)
{
	auto* contents = new FrameContents();
	contents->AllocRoot<BuildCallback>()->buildFunc = buildFunc;
	contents->BuildRoot();
	SetFrameContents(contents);
	ownsContents = true;
}

} // ui
