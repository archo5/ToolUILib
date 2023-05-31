
#include "System.h"

#include "../Core/Logging.h"

#include <algorithm>


namespace ui {

extern uint32_t g_curLayoutFrame;
FrameContents* g_curSystem;

namespace _ {
UIContainer* g_curContainer;
} // _
using namespace _;

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
		if (isRootBuildableOwned)
			DeleteUIObject(rootBuildable);
		rootBuildable = nullptr;
		isRootBuildableOwned = false;
	}
}

void UIContainer::ProcessSingleBuildable(Buildable* curB)
{
	bool oldEnabled = imm::SetEnabled(!(curB->flags & UIObject_IsDisabled));

	objectStackSize = 0;
	_Push(curB);
	_curObjectList = &curB->_objList;
	_curObjectList->BeginAllocations();

	if (CanLogDebug(LOG_UISYS))
		LogDebug(LOG_UISYS, "building %s", typeid(*curB).name());

	_curBuildable = curB;
	curB->_lastBuildFrameID = _lastBuildFrameID;

	curB->ClearLocalEventHandlers();

	// do not run old dtors before build (so that mid-build all data is still valid)
	// but have the space cleaned out for the new dtors
	decltype(Buildable::_deferredDestructors) oldDDs;
	std::swap(oldDDs, curB->_deferredDestructors);

	curB->DetachChildren(false);

	curB->Build();

	while (oldDDs.Size())
	{
		oldDDs.Last()();
		oldDDs.RemoveLast();
	}

	_curObjectList->EndAllocations();
	_curObjectList = nullptr;
	_curBuildable = nullptr;

	if (objectStackSize > 1)
	{
		LogWarn(LOG_UISYS, "elements not popped: %d (after building %s)", objectStackSize - 1, typeid(*curB).name());
		while (objectStackSize > 1)
		{
			Pop();
		}
	}

	_Pop(); // root

	imm::SetEnabled(oldEnabled);
}

static void CopyBuildablesFromSetWithoutChildren(Array<Buildable*>& outArr, const HashSet<Buildable*>& set)
{
	for (auto* B : set)
	{
		auto* p = B->parent;
		while (p)
		{
			if (set.Contains(static_cast<Buildable*>(p)))
				break;
			p = p->parent;
		}
		if (p)
			continue;
		outArr.Append(B);
	}
}

double hqtime();
void UIContainer::ProcessBuildStack()
{
	if (pendingBuildSet.IsEmpty())
		return;

	// the rules:
	//
	// - any built element triggers the in-frame subsequent rebuild of all children in entirety always
	//   this ensures that:
	//   - child buildables don't contain elements referring to outdated data
	//   - child buildables contain all the elements they're meant to contain
	//
	// - child elements are rebuilt after the full rebuild of their respective parents
	//   (as opposed to building child elements inline)
	//   this ensures that:
	//   - child buildables will have the data assigned to them after construction by the parent ..
	//     .. without any explicit calls in user code indicating when all the data has been assigned
	//
	// - each root-nearest buildable is built just once per "frame"/loop, rebuilds are queued for the next frame
	//   (eventual parent rebuilds however force the rebuilding of all children)
	//   this ensures that:
	//   - there are no infinite loops caused by persistent rebuilding of any buildable
	//   - all child buildables can still be built (unlike with blanket at-work queueing redirections)

	LogDebug(LOG_UISYS, " ---- processing node BUILD stack ----");
	//_lastBuildFrameID++;
	double t = hqtime();

	TmpEdit<decltype(g_curSystem)> tmp(g_curSystem, owner);
	TmpEdit<decltype(g_curContainer)> tmp2(g_curContainer, this);

	// TODO reuse across frames?
	Array<Buildable*> bldQueue;
	while (pendingBuildSet.NotEmpty())
	{
		// skip child builds when parent buildables are rebuilt again
		// this avoids the case where the deletion of data represented in a child buildable would be accessed after the parent deleting it
		bldQueue.Clear();
		CopyBuildablesFromSetWithoutChildren(bldQueue, pendingBuildSet);
		pendingBuildSet.Clear();

		for (Buildable* curB : bldQueue)
		{
			ProcessSingleBuildable(curB);
		}
	}

	for (Buildable* B : pendingNextBuildSet)
		pendingBuildSet.Insert(B);
	pendingNextBuildSet.Clear();

	pendingDeactivationSet.Flush();

	LogDebug(LOG_UISYS, "build %" PRIu64 " time: %.3f ms", _lastBuildFrameID, (hqtime() - t) * 1000);
	_lastBuildFrameID++;
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

void UIContainer::_BuildUsing(Buildable* B, bool transferOwnership)
{
	B->_AttachToFrameContents(owner);

	rootBuildable = B;
	isRootBuildableOwned = transferOwnership;

	assert(pendingBuildSet.IsEmpty());
	pendingBuildSet.Clear(); // TODO: is this needed?

	B->_lastBuildFrameID = _lastBuildFrameID - 1;
	UI_DEBUG_FLOW(printf("add %p to build set [add]\n", B));
	pendingBuildSet.Insert(B);

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

TextElement& NewText(StringView s)
{
	return New<TextElement>().SetText(s);
}

TextElement& NewTextVA(const char* fmt, va_list args)
{
	return NewText(FormatVA(fmt, args));
}

TextElement& NewTextf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	auto str = FormatVA(fmt, args);
	va_end(args);
	return NewText(str);
}

TextElement& Text(StringView s)
{
	auto& t = NewText(s);
	Add(t);
	return t;
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


void Overlays::Register(OverlayElement* oe)
{
	mapped.Insert(oe);
	sortedOutdated = true;
}

void Overlays::Unregister(OverlayElement* oe)
{
	mapped.Remove(oe);
	sortedOutdated = true;
}

void Overlays::UpdateSorted()
{
	if (!sortedOutdated)
		return;

	sorted.Clear();
	sorted.Reserve(mapped.size());
	for (OverlayElement* oe : mapped)
		sorted.Append(oe);
	std::sort(sorted.begin(), sorted.end(), [](const OverlayElement* a, const OverlayElement* b) { return a->_depth < b->_depth; });
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

void FrameContents::BuildRoot(Buildable* B, bool transferOwnership)
{
	container._BuildUsing(B, transferOwnership);
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
		_frameContents->container.rootBuildable->PerformLayout(GetFinalRect(), _rcvdLayoutInfo);
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
	contents->BuildRoot(cb, true);
	SetFrameContents(contents);
	_ownsContents = true;
}

} // ui
