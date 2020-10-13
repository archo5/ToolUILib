
#include "TreeEditor.h"

#include "../Model/Menu.h"

#include <algorithm>


namespace ui {

static void _RemoveChildrenFromListRecursive(ITree* tree, ItemLoc* nodes, size_t count, ItemLoc parent)
{
	for (auto ch = tree->GetFirstChild(parent); !tree->AtEnd(ch); ch = tree->GetNext(ch))
	{
		bool found = false;
		for (size_t j = 0; j < count; j++)
		{
			if (ch == nodes[j])
			{
				found = true;
				break;
			}
		}
	}
}

void ITree::IndexSort(ItemLoc* nodes, size_t count)
{
	if (GetFeatureFlags() & HasChildArray)
	{
		std::sort(nodes, nodes + count, [](const ItemLoc& a, const ItemLoc& b)
		{
			return a.index < b.index;
		});
	}
}

void ITree::RemoveChildrenFromList(ItemLoc* nodes, size_t count)
{
	if (GetFeatureFlags() & CanGetParent)
	{
		for (size_t i = 0; i < count; i++)
		{
			bool found = false;
			for (auto n = GetParent(nodes[i]); !AtEnd(n); n = GetParent(n))
			{
				for (size_t j = 0; j < count; j++)
				{
					if (n == nodes[j])
					{
						found = true;
						break;
					}
				}
				if (found)
					break;
			}

			if (found)
				nodes[i--] = nodes[--count];
		}
	}
	else
	{
		for (size_t i = 0; i < count; i++)
		{
			_RemoveChildrenFromListRecursive(this, nodes, count, nodes[i]);
		}
	}
}

void ITree::RemoveAll(ItemLoc* nodes, size_t count)
{
	RemoveChildrenFromList(nodes, count);
	IndexSort(nodes, count);
	for (size_t i = count; i < count; )
	{
		Remove(nodes[--i]);
	}
}

void ITree::DuplicateAll(ItemLoc* nodes, size_t count)
{
	RemoveChildrenFromList(nodes, count);
	IndexSort(nodes, count);
	for (size_t i = 0; i < count; i++)
	{
		Duplicate(nodes[i]);
	}
}


void TreeItemElement::OnInit()
{
	Selectable::OnInit();
	SetFlag(UIObject_DB_Draggable, true);
	auto s = GetStyle();
	s.SetLayout(style::layouts::StackExpand());
	s.SetPadding(0, 0, 0, 16);
}

void TreeItemElement::OnEvent(UIEvent& e)
{
	Selectable::OnEvent(e);

	if (e.type == UIEventType::ContextMenu)
		ContextMenu();

	if (e.context->DragCheck(e, UIMouseButton::Left))
	{
		ui::DragDrop::SetData(new TreeDragData(treeEd, { node }));
		e.context->SetKeyboardFocus(nullptr);
		e.context->ReleaseMouse();
	}
}

void TreeItemElement::ContextMenu()
{
	auto& CM = ContextMenu::Get();
	CM.Add("Duplicate") = [this]() { treeEd->GetTree()->Duplicate(node); RerenderNode(); };
	CM.Add("Remove") = [this]() { treeEd->GetTree()->Remove(node); RerenderNode(); };
	if (auto* cms = treeEd->GetContextMenuSource())
		cms->FillItemContextMenu(CM, node, 0);
}

void TreeItemElement::Init(TreeEditor* te, ItemLoc n)
{
	treeEd = te;
	node = n;

	bool dragging = false;
	if (auto* dd = ui::DragDrop::GetData<TreeDragData>())
		if (dd->nodes.size() == 1 && dd->nodes[0] == n && dd->scope == te)
			dragging = true;

	Selectable::Init(dragging);
}


void TreeEditor::Render(UIContainer* ctx)
{
	auto s = ctx->Push<ListBox>()->GetStyle();
	s.SetMinHeight(22);
	s.SetBoxSizing(style::BoxSizing::ContentBox);

	OnBuildList(ctx, _tree->GetFirstRoot());

	ctx->Pop();
}

void TreeEditor::OnEvent(UIEvent& e)
{
	Node::OnEvent(e);

	if (e.type == UIEventType::ContextMenu)
	{
		if (auto* cms = GetContextMenuSource())
			cms->FillListContextMenu(ContextMenu::Get());
	}
}

void TreeEditor::OnBuildChildList(UIContainer* ctx, ItemLoc firstChild)
{
	ctx->PushBox().GetStyle().SetPaddingLeft(8);

	OnBuildList(ctx, firstChild);

	ctx->Pop();
}

void TreeEditor::OnBuildList(UIContainer* ctx, ItemLoc firstNode)
{
	for (auto node = firstNode; !_tree->AtEnd(node); node = _tree->GetNext(node))
	{
		ctx->Push<TreeItemElement>()->Init(this, node);

		OnBuildItem(ctx, node);

		if (showDeleteButton)
			OnBuildDeleteButton(ctx, node);

		ctx->Pop();

		auto firstChild = _tree->GetFirstChild(node);
		if (!_tree->AtEnd(firstChild))
		{
			OnBuildChildList(ctx, firstChild);
		}
	}
}

void TreeEditor::OnBuildItem(UIContainer* ctx, ItemLoc node)
{
	if (itemUICallback)
		itemUICallback(ctx, this, node);
}

void TreeEditor::OnBuildDeleteButton(UIContainer* ctx, ItemLoc node)
{
	auto& delBtn = *ctx->MakeWithText<ui::Button>("X");
	delBtn + ui::Width(20);
	delBtn + ui::EventHandler(UIEventType::Activate, [this, node](UIEvent&) { GetTree()->Remove(node); Rerender(); });
}

TreeEditor& TreeEditor::SetTree(ITree* t)
{
	_tree = t;
	return *this;
}

TreeEditor& TreeEditor::SetContextMenuSource(IListContextMenuSource* src)
{
	_ctxMenuSrc = src;
	return *this;
}

} // ui
