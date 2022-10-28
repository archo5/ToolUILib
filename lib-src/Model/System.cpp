
#include "System.h"

#include "../Core/Logging.h"

#include <algorithm>


namespace ui {

extern uint32_t g_curLayoutFrame;
FrameContents* g_curSystem;
UIContainer* g_curContainer;

LogCategory LOG_UISYS("UISys");


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
	stack.Clear();
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
	stack.Append(n);
	n->flags |= flag;
}

void UIObjectDirtyStack::OnDestroy(UIObject* n)
{
	if (!(n->flags & flag))
	{
		CHECK_NOT_IN_DIRTY_STACK(n);
		return;
	}
	stack.UnorderedRemoveFirstOf(n);
	CHECK_NOT_IN_DIRTY_STACK(n);
}

UIObject* UIObjectDirtyStack::Pop()
{
	assert(stack.size() > 0);
	UIObject* n = stack.Last();
	stack.RemoveLast();
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
	stack.UnorderedRemoveAt(n);

	CHECK_NOT_IN_DIRTY_STACK(src);
}


void UIContainer::Free()
{
	if (rootBuildable)
	{
		rootBuildable->_DetachFromFrameContents();
		DeleteUIObject(rootBuildable);
		rootBuildable = nullptr;
	}
}

double hqtime();
void UIContainer::ProcessBuildStack()
{
	if (buildStack.ContainsAny())
	{
		LogDebug(LOG_UISYS, " ---- processing node BUILD stack ----");
		_lastBuildFrameID++;
	}
	else
		return;

	double t = hqtime();

	TmpEdit<decltype(g_curSystem)> tmp(g_curSystem, owner);
	TmpEdit<decltype(g_curContainer)> tmp2(g_curContainer, this);

	buildStack.RemoveChildren();

	while (buildStack.ContainsAny())
	{
		Buildable* currentBuildable = static_cast<Buildable*>(buildStack.Pop());

		bool oldEnabled = imm::SetEnabled(!(currentBuildable->flags & UIObject_IsDisabled));

		objectStackSize = 0;
		_Push(currentBuildable);
		_curObjectList = &currentBuildable->_objList;
		_curObjectList->BeginAllocations();

		if (CanLogDebug(LOG_UISYS))
			LogDebug(LOG_UISYS, "building %s", typeid(*currentBuildable).name());

		_curBuildable = currentBuildable;
		currentBuildable->_lastBuildFrameID = _lastBuildFrameID;

		currentBuildable->ClearLocalEventHandlers();

		// do not run old dtors before build (so that mid-build all data is still valid)
		// but have the space cleaned out for the new dtors
		decltype(Buildable::_deferredDestructors) oldDDs;
		std::swap(oldDDs, currentBuildable->_deferredDestructors);

		currentBuildable->DetachChildren(false);

		currentBuildable->Build();

		while (oldDDs.size())
		{
			oldDDs.Last()();
			oldDDs.RemoveLast();
		}

		_curObjectList->EndAllocations();
		_curObjectList = nullptr;
		_curBuildable = nullptr;

		if (objectStackSize > 1)
		{
			LogWarn(LOG_UISYS, "elements not popped: %d (after building %s)", objectStackSize - 1, typeid(*currentBuildable).name());
			while (objectStackSize > 1)
			{
				Pop();
			}
		}

		_Pop(); // root

		imm::SetEnabled(oldEnabled);
	}

	buildStack.Swap(nextFrameBuildStack);
	_lastBuildFrameID++;

	pendingDeactivationSet.Flush();

	LogDebug(LOG_UISYS, "build time: %.3f ms", (hqtime() - t) * 1000);
}

void UIContainer::ProcessLayoutStack()
{
	if (layoutStack.ContainsAny())
	{
		LogDebug(LOG_UISYS, " ---- processing node LAYOUT stack (%zu items) ----", layoutStack.stack.size());
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

	assert(layoutStack.stack.NotEmpty());
	g_curLayoutFrame++;

	// single pass
	for (UIObject* obj : layoutStack.stack)
	{
		double t0 = hqtime();

		// TODO how to restart layout?
		obj->RedoLayout();

		double t1 = hqtime();
		LogDebug(LOG_UISYS, "relayout %s @ %p took %.3f ms", typeid(*obj).name(), obj, (t1 - t0) * 1000);
	}
	layoutStack.Clear();
}

void UIContainer::_BuildUsing(Buildable* n)
{
	n->_AttachToFrameContents(owner);

	rootBuildable = n;

	assert(!buildStack.ContainsAny());
	buildStack.ClearWithoutFlags(); // TODO: is this needed?

	AddToBuildStack(rootBuildable);

	ProcessBuildStack();
	ProcessLayoutStack();
}

void UIContainer::_Push(UIObject* obj)
{
	objectStack[objectStackSize] = obj;
	objectStackSize++;
}

void UIContainer::_Pop()
{
	objectStackSize--;
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

	sorted.Clear();
	sorted.Reserve(mapped.size());
	for (const auto& kvp : mapped)
		sorted.Append({ kvp.key, kvp.value.depth });
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

void FrameContents::BuildRoot(Buildable* B)
{
	container._BuildUsing(B);
}


void InlineFrame::OnReset()
{
	if (_frameContents)
	{
		if (_ownsContents)
		{
			delete _frameContents;
		}
		else
		{
			_frameContents->owningFrame = nullptr;
			_frameContents->nativeWindow = nullptr;
		}
	}
	_frameContents = nullptr;
	_ownsContents = false;
}

void InlineFrame::Build()
{
}

void InlineFrame::OnEvent(Event& ev)
{
	if (_frameContents)
	{
		// pass events
		if (ev.type == EventType::ButtonDown || ev.type == EventType::ButtonUp)
			_frameContents->eventSystem.OnMouseButton(ev.type == EventType::ButtonDown, ev.GetButton(), ev.position, ev.modifiers);
		if (ev.type == EventType::MouseMove || ev.type == EventType::MouseLeave)
			_frameContents->eventSystem.OnMouseMove(ev.position, ev.modifiers);
	}
	Buildable::OnEvent(ev);
}

void InlineFrame::OnPaint(const UIPaintContext& ctx)
{
	if (_frameContents &&
		_frameContents->container.rootBuildable)
		_frameContents->container.rootBuildable->OnPaint({ ctx, {} });
}

Rangef InlineFrame::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	return Rangef::AtLeast(100); // default width
}

Rangef InlineFrame::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	return Rangef::AtLeast(100); // default height
}

void InlineFrame::OnLayoutChanged()
{
	if (_frameContents &&
		_frameContents->container.rootBuildable)
		_frameContents->container.rootBuildable->PerformLayout(GetFinalRect());
}

void InlineFrame::SetFrameContents(FrameContents* contents)
{
	if (_frameContents)
	{
		if (_ownsContents)
		{
			delete _frameContents;
		}
		else
		{
			_frameContents->owningFrame = nullptr;
			_frameContents->nativeWindow = nullptr;
		}
	}
	if (contents)
	{
		if (contents->owningFrame)
		{
			contents->owningFrame->_frameContents = nullptr;
		}
		contents->nativeWindow = GetNativeWindow();
		contents->owningFrame = this;
	}
	_frameContents = contents;
	_ownsContents = false;
	_OnChangeStyle();
}

void InlineFrame::CreateFrameContents(std::function<void()> buildFunc)
{
	auto* contents = new FrameContents();
	auto* cb = CreateUIObject<BuildCallback>();
	cb->buildFunc = buildFunc;
	contents->BuildRoot(cb);
	SetFrameContents(contents);
	_ownsContents = true;
}

} // ui
