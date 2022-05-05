
#include "Docking.h"

#include "System.h"

#include "../Elements/TabbedPanel.h"
#include "../Elements/SplitPane.h"


namespace ui {

DockableContentsContainer::DockableContentsContainer(DockableContents* C) : contents(C)
{
	frameContents = new FrameContents;
	frameContents->BuildRoot(C);
}

void DockingNode::Build()
{
	if (isLeaf)
	{
		auto& tp = Push<TabbedPanel>();
		tp.allowDragReorder = true;
		tp.allowDragRemove = true;
		tp.showCloseButton = true;

		for (auto& tab : tabs)
			tp.AddEnumTab<DockableContentsContainer*>(tab->contents->GetTitle(), tab);

		tp.SetActiveTabByEnumValue<DockableContentsContainer*>(curActiveTab);
		tp.HandleEvent(&tp, ui::EventType::SelectionChange) = [this, &tp](ui::Event&)
		{
			curActiveTab = tp.GetCurrentTabEnumValue<DockableContentsContainer*>(0);
		};
		auto* CB = GetCurrentBuildable();
		tp.HandleEvent(&tp, ui::EventType::Change) = [this, &tp, CB](ui::Event& e)
		{
			if (e.arg1 == UINTPTR_MAX)
			{
				e.context->ReleaseMouse();
				main->_PullOutTab(this, e.arg0);
				CB->Rebuild();
				return;
			}
			uintptr_t inc = e.arg1 > e.arg0 ? 1 : UINTPTR_MAX;
			for (uintptr_t i = e.arg0; i < e.arg1; i += inc)
			{
				std::swap(tabs[i], tabs[i + inc]);
			}
		};
		tp.onClose = [this, CB](size_t num, uintptr_t id)
		{
			main->_CloseTab(this, num);
			CB->Rebuild();
		};

		if (curActiveTab)
			Make<InlineFrame>().SetFrameContents(curActiveTab->frameContents);

		Pop();
	}
	else
	{
		Push<SplitPane>().Init(splitDir, splits.data(), splits.size());

		for (auto& cn : childNodes)
			cn->Build();

		Pop();
	}
}

void DockingSubwindow::OnBuild()
{
	_root = GetCurrentBuildable();
	if (_dragging)
		_root->system->eventSystem.CaptureMouse(_root);
	rootNode->Build();
	GetCurrentBuildable()->HandleEvent() = [this](Event& e)
	{
		printf("%s\n", EventTypeToBaseString(e.type));
		if (_dragging)
		{
			if (e.type == EventType::MouseMove)
			{
				auto csp = platform::GetCursorScreenPos();
				SetOuterPosition(csp + _dragCWPDelta);
			}
			else if (e.type == EventType::ButtonUp && e.GetButton() == MouseButton::Left)
			{
				e.context->ReleaseMouse();
				_dragging = false;
			}
		}
	};
}

void DockingSubwindow::StartDrag()
{
	_dragging = true;
	auto csp = platform::GetCursorScreenPos();
	auto wp = GetOuterPosition();
	_dragCWPDelta = wp - csp;

	// force-transfer capture to this window
	CaptureMouse();

	if (_root)
		_root->system->eventSystem.CaptureMouse(_root);
}

void DockingWindowContentBuilder::Build()
{
	TEMP_LAYOUT_MODE = FILLER;

	_root->Build();
}

void DockingMainArea::Build()
{
	TEMP_LAYOUT_MODE = FILLER;

	Make<DockingWindowContentBuilder>()._root = _mainAreaRootNode;
}

void DockingMainArea::_PullOutTab(DockingNode* node, size_t tabID)
{
	DockableContentsContainerHandle dcch = node->tabs[tabID];
	AABB2i rect = dcch->frameContents->owningFrame->parent->GetFinalRect().Cast<int>();
	auto wpos = dcch->frameContents->nativeWindow->GetInnerPosition();
	assert(dcch);

	node->tabs.erase(node->tabs.begin() + tabID);
	if (node->tabs.size() == 0)
	{
		node->curActiveTab = nullptr;
		// TODO: cascade of destruction
	}
	else if (node->curActiveTab == dcch)
		node->curActiveTab = node->tabs[tabID ? tabID - 1 : 0];

	DockingNode* DN = new DockingNode;
	DN->isLeaf = true;
	DN->main = this;
	DN->tabs.push_back(dcch);
	DN->curActiveTab = dcch;

	DockingSubwindow* DSW = new DockingSubwindow;
	DSW->rootNode = DN;
	DN->subwindow = DSW;

	_subwindows.push_back(DSW);
	AABB2i finalClientRect = rect.MoveBy(wpos.x, wpos.y);
	DSW->SetInnerRect(finalClientRect);
	DSW->SetVisible(true);
	DSW->StartDrag();

	DragDrop::SetData(nullptr);

	Rebuild();
}

void DockingMainArea::_CloseTab(DockingNode* node, size_t tabID)
{
	DockableContentsContainerHandle dcch = node->tabs[tabID];

	node->tabs.erase(node->tabs.begin() + tabID);
	if (node->tabs.size() == 0)
	{
		node->curActiveTab = nullptr;
		// TODO: cascade of destruction
	}
	else if (node->curActiveTab == dcch)
		node->curActiveTab = node->tabs[tabID ? tabID - 1 : 0];
}

} // ui
