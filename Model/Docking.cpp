
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

void DockingNode::SetSubwindow(DockingSubwindow* dsw)
{
	subwindow = dsw;
	if (!isLeaf)
	{
		for (auto& cn : childNodes)
			cn->SetSubwindow(dsw);
	}
}

void DockingNode::Build()
{
	if (isLeaf)
	{
		bool topLevel = tabs.size() <= 1 && !parentNode;

		auto& tp = Push<TabbedPanel>();
		tp.allowDragReorder = true;
		if (!topLevel)
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
				CB->Rebuild();
				// could delete this node
				main->_PullOutTab(this, e.arg0);
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
			CB->Rebuild();
			// could delete this node
			main->_CloseTab(this, num);
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
		//printf("%s\n", EventTypeToBaseString(e.type));
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
				SetHitTestEnabled(true);
			}
		}
		if (rootNode->isLeaf && rootNode->tabs.size() <= 1)
		{
			switch (e.type)
			{
			case EventType::DragStart:
				StartDrag();
				break;
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

	SetHitTestEnabled(false);

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
		Application::PushEvent([this, node]()
		{
			_DeleteNode(node);
		});
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
	DSW->SetStyle(WS_Resizable);
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
		Application::PushEvent([this, node]()
		{
			_DeleteNode(node);
		});
	}
	else if (node->curActiveTab == dcch)
		node->curActiveTab = node->tabs[tabID ? tabID - 1 : 0];
}

void DockingMainArea::_DeleteNode(DockingNode* node)
{
	DockingNode* cch = node;
	while (DockingNode* P = cch->parentNode)
	{
		for (size_t i = 0; i < P->childNodes.size(); i++)
		{
			if (P->childNodes[i] == cch)
			{
				P->childNodes.erase(P->childNodes.begin() + i);
				// TODO rebalance splits
				if (!P->splits.empty())
					P->splits.pop_back();
				break;
			}
		}
		if (P->childNodes.empty())
			break;
		cch = P;
	}

	// empty root node, close its window
	if (!cch->parentNode && cch->subwindow && (cch->isLeaf ? cch->tabs.empty() : cch->childNodes.empty()))
	{
		for (size_t i = 0; i < _subwindows.size(); i++)
		{
			if (_subwindows[i] == cch->subwindow)
			{
				_subwindows.erase(_subwindows.begin() + i);
				break;
			}
		}
	}
}

DockingMainArea& DockingMainArea::SetSource(DockableContentsSource* dcs)
{
	source = dcs;
	return *this;
}

void DockingMainArea::Clear()
{
	RemoveSubwindows();
	ClearMainArea();
}

void DockingMainArea::ClearMainArea()
{
	_mainAreaRootNode = nullptr;
	Rebuild();
}

void DockingMainArea::RemoveSubwindows()
{
	_subwindows.clear();
	Rebuild();
}

void DockingMainArea::SetMainAreaContents(const DockDefNode& node)
{
	ClearMainArea();
	_mainAreaRootNode = node.Construct(this);
}

void DockingMainArea::AddSubwindow(const DockDefNode& node)
{
	DockingSubwindow* DSW = new DockingSubwindow;
	DSW->rootNode = node.Construct(this);
	DSW->rootNode->SetSubwindow(DSW);
	_subwindows.push_back(DSW);
	Rebuild();
}

namespace dockdef {

DockingNodeHandle Split::Construct(DockingMainArea* main) const
{
	auto* node = new ui::DockingNode;
	node->isLeaf = false;
	node->main = main;
	node->splitDir = GetSplitDirection();

	for (auto& cn : children)
	{
		auto ccnh = cn.node->Construct(main);
		ccnh->parentNode = node;

		node->childNodes.push_back(ccnh);
		if (&cn != &children.front())
			node->splits.push_back(cn.split);
	}

	return node;
}

DockingNodeHandle Tabs::Construct(DockingMainArea* main) const
{
	auto* node = new ui::DockingNode;
	node->isLeaf = true;
	node->main = main;

	for (auto tab : tabContentIDs)
	{
		auto* dc = main->source->GetDockableContentsByID(tab);
		if (!dc)
			continue;

		auto* dcc = new ui::DockableContentsContainer(dc);
		node->tabs.push_back(dcc);

		if (!node->curActiveTab)
			node->curActiveTab = dcc;
	}
	return node;
}

} // dockdef

} // ui
