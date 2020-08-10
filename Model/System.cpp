
#include "System.h"


void UIObjectDirtyStack::Add(UIObject* n)
{
	if (n->flags & flag)
		return;
	stack[size++] = n;
	assert(size < 128);
	n->flags |= flag;
}

void UIObjectDirtyStack::OnDestroy(UIObject* n)
{
	if (!(n->flags & flag))
		return;
	for (int i = 0; i < size; i++)
	{
		if (stack[i] == n)
		{
			if (i + 1 < size)
				stack[i] = stack[size - 1];
			size--;
			break;
		}
	}
}

UIObject* UIObjectDirtyStack::Pop()
{
	assert(size > 0);
	auto* n = stack[--size];
	n->flags &= ~flag;
	return n;
}

void UIObjectDirtyStack::RemoveChildren()
{
	for (int i = 0; i < size; i++)
	{
		auto* p = stack[i]->parent;
		while (p)
		{
			if (p->flags & flag)
				break;
			p = p->parent;
		}
		if (p)
		{
			stack[i]->flags &= ~flag;
			if (i + 1 < size)
				stack[i] = stack[size - 1];
			size--;
			i--;
		}
	}
}


void UIContainer::Free()
{
	if (rootNode)
	{
		_Destroy(rootNode);
		objectStack[0] = rootNode;
		objectStackSize = 1;
		ProcessObjectDeleteStack();
		rootNode = nullptr;
	}
}

void UIContainer::ProcessObjectDeleteStack(int first)
{
	while (objectStackSize > first)
	{
		auto* cur = objectStack[--objectStackSize];

		for (auto* n = cur->firstChild; n != nullptr; n = n->next)
			objectStack[objectStackSize++] = n;

		DEBUG_FLOW(printf("    deleting %p\n", cur));
		cur->system->eventSystem.OnDestroy(cur);
		nodeRenderStack.OnDestroy(cur);
		nextFrameNodeRenderStack.OnDestroy(cur);
		delete cur;
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

void UIContainer::ProcessNodeRenderStack()
{
	if (nodeRenderStack.ContainsAny())
	{
		DEBUG_FLOW(puts(" ---- processing node RENDER stack ----"));
		_lastRenderedFrameID++;
	}
	else
		return;

	nodeRenderStack.RemoveChildren();

	while (nodeRenderStack.ContainsAny())
	{
		ui::Node* currentNode = static_cast<ui::Node*>(nodeRenderStack.Pop());

		objectStackSize = 0;
		_Push(currentNode, true);

		DEBUG_FLOW(printf("rendering %s\n", typeid(*currentNode).name()));
		_curNode = currentNode;
		currentNode->_lastRenderedFrameID = _lastRenderedFrameID;
		currentNode->ClearLocalEventHandlers();
		currentNode->_PerformDestructions();
		currentNode->Render(this);
		_curNode = nullptr;

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

	nodeRenderStack.Swap(nextFrameNodeRenderStack);
	_lastRenderedFrameID++;
}

void UIContainer::_BuildUsing(ui::Node* n)
{
	rootNode = n;
	assert(!nodeRenderStack.ContainsAny());
	nodeRenderStack.ClearWithoutFlags(); // TODO: is this needed?
	AddToRenderStack(rootNode);
	ProcessNodeRenderStack();
}

void UIContainer::_Push(UIObject* obj, bool isCurNode)
{
	objectStack[objectStackSize] = obj;
	objChildStack[objectStackSize] = obj->firstChild;
	objectStackSize++;
}

void UIContainer::_Destroy(UIObject* obj)
{
	DEBUG_FLOW(printf("    destroy %p\n", obj));
	obj->_livenessToken.SetAlive(false);
	obj->OnDestroy();
	for (auto* ch = obj->firstChild; ch; ch = ch->next)
		_Destroy(ch);
}

void UIContainer::_Pop()
{
	objectStack[objectStackSize - 1]->OnCompleteStructure();
	objectStack[objectStackSize - 1]->_livenessToken.SetAlive(true);
	if (objChildStack[objectStackSize - 1])
	{
		// remove leftover children
		DeleteObjectsStartingFrom(objChildStack[objectStackSize - 1]);
	}
	objectStackSize--;
}

ui::NativeWindowBase* UIContainer::GetNativeWindow() const
{
	return owner->nativeWindow;
}


namespace ui {

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

void InlineFrameNode::OnDestroy()
{
	if (ownsContents && frameContents)
	{
		frameContents->container.Free();
		delete frameContents;
		frameContents = nullptr;
		ownsContents = false;
	}
}

void InlineFrameNode::Render(UIContainer* ctx)
{
}

void InlineFrameNode::OnEvent(UIEvent& ev)
{
	if (frameContents)
	{
		// pass events
		if (ev.type == UIEventType::ButtonDown || ev.type == UIEventType::ButtonUp)
			frameContents->eventSystem.OnMouseButton(ev.type == UIEventType::ButtonDown, ev.GetButton(), ev.x, ev.y);
		if (ev.type == UIEventType::MouseMove)
			frameContents->eventSystem.OnMouseMove(ev.x, ev.y);
	}
	Node::OnEvent(ev);
}

void InlineFrameNode::OnPaint()
{
	if (frameContents &&
		frameContents->container.rootNode)
		frameContents->container.rootNode->OnPaint();
}

float InlineFrameNode::CalcEstimatedWidth(const Size<float>& containerSize, style::EstSizeType type)
{
	return 100; // default width
}

float InlineFrameNode::CalcEstimatedHeight(const Size<float>& containerSize, style::EstSizeType type)
{
	return 100; // default height
}

void InlineFrameNode::OnLayoutChanged()
{
	if (frameContents &&
		frameContents->container.rootNode)
		frameContents->container.rootNode->OnLayout(finalRectC, finalRectC.GetSize());
}

void InlineFrameNode::SetFrameContents(FrameContents* contents)
{
	if (ownsContents && frameContents)
	{
		frameContents->container.Free();
		delete frameContents;
	}
	frameContents = contents;
	contents->nativeWindow = GetNativeWindow();
	ownsContents = false;
}

void InlineFrameNode::CreateFrameContents(std::function<void(UIContainer* ctx)> renderFunc)
{
	if (ownsContents && frameContents)
	{
		frameContents->container.Free();
		delete frameContents;
	}
	frameContents = new FrameContents();
	frameContents->nativeWindow = GetNativeWindow();
	ownsContents = true;

	auto& cont = frameContents->container;
	auto& evsys = frameContents->eventSystem;

	auto* N = cont.AllocIfDifferent<RenderNode>(cont.rootNode);
	N->renderFunc = renderFunc;
	cont._BuildUsing(N);
	evsys.RecomputeLayout();
}

} // ui
