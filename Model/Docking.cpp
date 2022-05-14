
#include "Docking.h"

#include "System.h"

#include "../Elements/TabbedPanel.h"
#include "../Elements/SplitPane.h"


namespace ui {

static DataCategoryTag DCT_DockingRootUpdated[1];
static DataCategoryTag DCT_DockingNodeUpdated[1];

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

DockingInsertionTarget DockingNode::FindInsertionTarget(Vec2f pos)
{
	if (!rectSource)
		return {};

	auto rect = rectSource->GetFinalRect();
	if (!rect.Contains(pos))
		return {};

	if (!isLeaf)
	{
		for (auto& cn : childNodes)
		{
			auto ret = cn->FindInsertionTarget(pos);
			if (ret.node)
				return ret;
		}
		return {};
	}

	float margin = min(rect.GetWidth(), rect.GetHeight()) * 0.25f;
	auto inner = rect.ShrinkBy(UIRect::UniformBorder(margin));
	if (inner.Contains(pos))
		return { this, DockingInsertionSide_Here };

	float distL = fabsf(rect.x0 - pos.x);
	float distR = fabsf(rect.x1 - pos.x);
	float distT = fabsf(rect.y0 - pos.y);
	float distB = fabsf(rect.y1 - pos.y);
	float distH = min(distL, distR);
	float distV = min(distT, distB);

	return
	{
		this,
		distH < distV
		? distL < distR ? DockingInsertionSide_Left : DockingInsertionSide_Right
		: distT < distB ? DockingInsertionSide_Above : DockingInsertionSide_Below
	};
}

void DockingNode::Build()
{
	GetCurrentBuildable()->Subscribe(DCT_DockingNodeUpdated, this);

	if (isLeaf)
	{
		bool topLevel = tabs.size() <= 1 && !parentNode;

		auto& tp = Push<TabbedPanel>();
		tp.allowDragReorder = true;
		if (!topLevel)
			tp.allowDragRemove = true;
		tp.showCloseButton = true;

		rectSource = &tp;

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
				e.StopPropagation();
				return;
			}
			uintptr_t inc = e.arg1 > e.arg0 ? 1 : UINTPTR_MAX;
			for (uintptr_t i = e.arg0; i != e.arg1; i += inc)
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
		rectSource = &Push<SplitPane>().Init(splitDir, splits.data(), splits.size());

		for (auto& cn : childNodes)
			cn->Build();

		Pop();
	}
}

void DockingSubwindow::OnBuild()
{
	auto* B = GetCurrentBuildable();
	_root = B;
	B->Subscribe(DCT_DockingRootUpdated, &rootNode);
	if (_dragging)
		B->system->eventSystem.CaptureMouse(B);
	//rootNode->Build();
	Make<DockingWindowContentBuilder>()._root = rootNode;
	B->HandleEvent() = [this](Event& e)
	{
		OnBuildableEvent(e);
	};
}

void DockingSubwindow::OnBuildableEvent(Event& e)
{
	//printf("%s\n", EventTypeToBaseString(e.type));
	if (_dragging)
	{
		if (e.type == EventType::MouseMove)
		{
			auto csp = platform::GetCursorScreenPos();
			SetOuterPosition(csp + _dragCWPDelta);

			rootNode->main->_UpdateInsertionTarget(csp);
		}
		else if (e.type == EventType::ButtonUp && e.GetButton() == MouseButton::Left)
		{
			e.context->ReleaseMouse();
			_dragging = false;
			SetHitTestEnabled(true);

			Application::PushEvent([this]()
			{
				rootNode->main->_FinishInsertion(this);
			});
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

void DockingWindowContentBuilder::OnPaint(const UIPaintContext& ctx)
{
	Buildable::OnPaint(ctx);

	auto tgt = _root->main->_insTarget;
	if (tgt.node && tgt.node->subwindow == _root->subwindow)
	{
		if (auto* rs = tgt.node->rectSource.Get())
		{
			// TODO
			auto r = rs->GetFinalRect();
			float margin = min(r.GetWidth(), r.GetHeight()) * 0.25f;
			auto q = r.ShrinkBy(UIRect::UniformBorder(margin));

			switch (tgt.tabOrSide)
			{
			case DockingInsertionSide_Here:
				draw::RectCol(q.x0, q.y0, q.x1, q.y1, Color4b(0, 64, 192, 64));
				break;
				// TODO diagonal edges?
			case DockingInsertionSide_Above:
				draw::RectCol(r.x0, r.y0, r.x1, q.y0, Color4b(0, 64, 192, 64));
				break;
			case DockingInsertionSide_Below:
				draw::RectCol(r.x0, q.y1, r.x1, r.y1, Color4b(0, 64, 192, 64));
				break;
			case DockingInsertionSide_Left:
				draw::RectCol(r.x0, r.y0, q.x0, r.y1, Color4b(0, 64, 192, 64));
				break;
			case DockingInsertionSide_Right:
				draw::RectCol(q.x1, r.y0, r.x1, r.y1, Color4b(0, 64, 192, 64));
				break;
			}
		}
	}
}

void DockingMainArea::Build()
{
	TEMP_LAYOUT_MODE = FILLER;

	Subscribe(DCT_DockingRootUpdated, &_mainAreaRootNode);
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
		if (!P->childNodes.empty())
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

void DockingMainArea::_UpdateInsertionTarget(Point2i screenCursorPos)
{
	auto* win = NativeWindowBase::FindFromScreenPos(screenCursorPos);

	DockingNode* root = nullptr;
	Point2i testPos = screenCursorPos;
	if (win)
	{
		if (win == GetNativeWindow())
		{
			root = _mainAreaRootNode;
			testPos -= GetNativeWindow()->GetInnerPosition();
		}
		for (auto& sw : _subwindows)
		{
			if (win == sw)
			{
				root = sw->rootNode;
				testPos -= sw->GetInnerPosition();
			}
		}
	}

	if (root)
		_insTarget = root->FindInsertionTarget(testPos.Cast<float>());
	else
		_ClearInsertionTarget();

	GetNativeWindow()->InvalidateAll();
	for (auto& sw : _subwindows)
		sw->InvalidateAll();
}

void DockingMainArea::_FinishInsertion(DockingSubwindow* dsw)
{
	if (auto tgt = _insTarget.node)
	{
		DockingNodeHandle insSWRoot = dsw->rootNode;
		assert(insSWRoot->isLeaf);
		assert(insSWRoot->tabs.size() == 1);

		DockableContentsContainerHandle tab = insSWRoot->tabs[0];

		// remove the window
		for (size_t i = 0; i < _subwindows.size(); i++)
		{
			if (_subwindows[i] == dsw)
			{
				_subwindows.erase(_subwindows.begin() + i);
				break;
			}
		}

		// tab insertion
		if (_insTarget.tabOrSide == DockingInsertionSide_Here || 
			_insTarget.tabOrSide >= 0)
		{
			int pos = _insTarget.tabOrSide == DockingInsertionSide_Here ? 0 : _insTarget.tabOrSide;

			tgt->tabs.insert(tgt->tabs.begin() + pos, tab);
			tgt->curActiveTab = tab;

			Notify(DCT_DockingNodeUpdated, tgt);
		}
		else // split insertion
		{
			insSWRoot->SetSubwindow(tgt->subwindow);

			Direction splitDir = _insTarget.tabOrSide == DockingInsertionSide_Left
				|| _insTarget.tabOrSide == DockingInsertionSide_Right
				? Direction::Horizontal : Direction::Vertical;

			auto* P = tgt->parentNode;
			// find the location of the target inside parent
			size_t idx = 0;
			if (P)
			{
				for (; idx < P->childNodes.size(); idx++)
				{
					if (P->childNodes[idx] == tgt)
						break;
				}
			}

			// need to make a new shared parent
			if (!P || P->splitDir != splitDir)
			{
				DockingNodeHandle mid = new DockingNode;
				mid->isLeaf = false;
				mid->main = this;
				mid->parentNode = P;
				mid->subwindow = tgt->subwindow;
				mid->splitDir = splitDir;
				mid->childNodes.push_back(tgt);

				tgt->parentNode = mid;

				if (P)
				{
					Notify(DCT_DockingNodeUpdated, P);
					P->childNodes[idx] = mid;
				}
				else
				{
					DockingNodeHandle& rootRef = tgt->subwindow ? tgt->subwindow->rootNode : _mainAreaRootNode;
					Notify(DCT_DockingRootUpdated, &rootRef);
					rootRef = mid;
				}

				P = mid;
				idx = 0;
			}
			else
				Notify(DCT_DockingNodeUpdated, P);

			bool insAfter = _insTarget.tabOrSide == DockingInsertionSide_Right
				|| _insTarget.tabOrSide == DockingInsertionSide_Below;
			size_t insIdx = insAfter ? idx + 1 : idx;

			P->childNodes.insert(P->childNodes.begin() + insIdx, insSWRoot);
			insSWRoot->parentNode = P;
			float before = idx == 0 ? 0 : P->splits[idx - 1];
			float after = idx >= P->splits.size() ? 1 : P->splits[idx];
			float splitVal = (before + after) * 0.5f;
			P->splits.insert(P->splits.begin() + idx, splitVal);
		}
	}

	_ClearInsertionTarget();
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
