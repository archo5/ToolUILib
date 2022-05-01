
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
		tp.showCloseButton = true;

		for (auto& tab : tabs)
			tp.AddEnumTab<DockableContentsContainer*>(tab->contents->GetTitle(), tab);

		tp.SetActiveTabByEnumValue<DockableContentsContainer*>(curActiveTab);
		tp.HandleEvent(&tp, ui::EventType::Commit) = [this, &tp](ui::Event&)
		{
			curActiveTab = tp.GetCurrentTabEnumValue<DockableContentsContainer*>(0);
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

} // ui
