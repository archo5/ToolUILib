
#include "TreeEditor.h"

#include "../Model/Menu.h"

#include <algorithm>


namespace ui {

static void _RemoveChildrenFromListRecursive(ITree* tree, ItemLoc* nodes, size_t& count, ItemLoc parent)
{
	for (auto ch = tree->GetFirstChild(parent); !tree->AtEnd(ch); ch = tree->GetNext(ch))
	{
		for (size_t j = 0; j < count; j++)
		{
			if (ch == nodes[j])
			{
				nodes[j] = nodes[--count];
				break;
			}
		}
		_RemoveChildrenFromListRecursive(tree, nodes, count, ch);
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

void ITree::RemoveChildrenFromList(ItemLoc* nodes, size_t& count)
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

void ITree::RemoveChildrenFromListOf(ItemLoc* nodes, size_t& count, ItemLoc parent)
{
	if (GetFeatureFlags() & CanGetParent)
	{
		for (size_t i = 0; i < count; i++)
		{
			bool found = false;
			for (auto n = GetParent(nodes[i]); !AtEnd(n); n = GetParent(n))
			{
				if (n == parent)
				{
					found = true;
					break;
				}
			}

			if (found)
				nodes[i--] = nodes[--count];
		}
	}
	else
	{
		_RemoveChildrenFromListRecursive(this, nodes, count, parent);
	}
}

void ITree::RemoveAll(TreePathRef* paths, size_t count)
{
	// TODO
	//RemoveChildrenFromList(paths, count);
	//IndexSort(paths, count);
	for (size_t i = count; i < count; )
	{
		Remove(paths[--i]);
	}
}

void ITree::DuplicateAll(TreePathRef* paths, size_t count)
{
	// TODO
	//RemoveChildrenFromList(paths, count);
	//IndexSort(paths, count);
	for (size_t i = 0; i < count; i++)
	{
		Duplicate(paths[i]);
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
	else if (e.type == UIEventType::DragMove)
	{
		if (auto* ddd = ui::DragDrop::GetData<TreeDragData>())
			if (ddd->scope == treeEd)
				treeEd->_OnDragMove(ddd, node, GetBorderRect(), e);
	}
	if (e.type == UIEventType::DragDrop)
	{
		if (auto* ddd = ui::DragDrop::GetData<TreeDragData>())
			if (ddd->scope == treeEd)
				treeEd->_OnDragDrop(ddd);
	}
	else if (e.type == UIEventType::DragLeave)
	{
		if (auto* ddd = ui::DragDrop::GetData<TreeDragData>())
			if (ddd->scope == treeEd)
				treeEd->_dragTargetLoc = {};
	}
}

void TreeItemElement::ContextMenu()
{
	auto& CM = ContextMenu::Get();
	CM.Add("Duplicate") = [this]() { treeEd->GetTree()->Duplicate(path); treeEd->_OnEdit(this); };
	CM.Add("Remove") = [this]() { treeEd->GetTree()->Remove(path); treeEd->_OnEdit(this); };
	if (auto* cms = treeEd->GetContextMenuSource())
		cms->FillItemContextMenu(CM, path, 0);
}

void TreeItemElement::Init(TreeEditor* te, ItemLoc n, const std::vector<uintptr_t>& p)
{
	treeEd = te;
	node = n;
	path = p;

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

	std::vector<uintptr_t> path;
	OnBuildList(ctx, _tree->GetFirstRoot(), path);

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

void TreeEditor::OnPaint()
{
	Node::OnPaint();

	if (_dragTargetLoc.IsValid())
	{
		auto r = _dragTargetLine;
		ui::draw::RectCol(r.x0, r.y0, r.x1, r.y1, ui::Color4f(0.1f, 0.7f, 0.9f, 0.6f));
	}
}

void TreeEditor::OnSerialize(IDataSerializer& s)
{
}

void TreeEditor::OnBuildChildList(UIContainer* ctx, ItemLoc firstChild, std::vector<uintptr_t>& path)
{
	ctx->PushBox().GetStyle().SetPaddingLeft(8);

	OnBuildList(ctx, firstChild, path);

	ctx->Pop();
}

void TreeEditor::OnBuildList(UIContainer* ctx, ItemLoc firstNode, std::vector<uintptr_t>& path)
{
	for (auto node = firstNode; !_tree->AtEnd(node); node = _tree->GetNext(node))
	{
		path.push_back(node.index);

		ctx->Push<TreeItemElement>()->Init(this, node, path);

		OnBuildItem(ctx, node, path);

		if (showDeleteButton)
			OnBuildDeleteButton(ctx);

		ctx->Pop();

		auto firstChild = _tree->GetFirstChild(node);
		if (!_tree->AtEnd(firstChild))
		{
			OnBuildChildList(ctx, firstChild, path);
		}

		path.pop_back();
	}
}

void TreeEditor::OnBuildItem(UIContainer* ctx, ItemLoc node, std::vector<uintptr_t>& path)
{
	if (itemUICallback)
		itemUICallback(ctx, this, node);
}

void TreeEditor::OnBuildDeleteButton(UIContainer* ctx)
{
	auto& delBtn = *ctx->MakeWithText<ui::Button>("X");
	delBtn + ui::Width(20);
	delBtn + ui::EventHandler(UIEventType::Activate, [this, &delBtn](UIEvent&)
	{
		GetTree()->Remove(delBtn.FindParentOfType<TreeItemElement>()->path);
		_OnEdit(this);
	});
}

TreeEditor& TreeEditor::SetTree(ITree* t)
{
	_tree = t;
	return *this;
}

TreeEditor& TreeEditor::SetContextMenuSource(ITreeContextMenuSource* src)
{
	_ctxMenuSrc = src;
	return *this;
}

void TreeEditor::_OnEdit(UIObject* who)
{
	system->eventSystem.OnChange(who);
	system->eventSystem.OnCommit(who);
	who->RerenderNode();
}

void TreeEditor::_OnDragMove(TreeDragData* tdd, ItemLoc item, const UIRect& rect, UIEvent& e)
{
	int level = 0; // TODO?
	auto& R = rect;
	if (e.y < lerp(R.y0, R.y1, 0.25f))
	{
		// above
		_dragTargetLine = UIRect{ R.x0 + level * 12, R.y0, R.x1, R.y0 }.ExtendBy(UIRect::UniformBorder(1));
		_dragTargetLoc = item;
		_dragTargetInsDir = TreeInsertMode::Before;
	}
	else if (e.y > lerp(R.y0, R.y1, 0.75f))
	{
		// below
		_dragTargetLine = UIRect{ R.x0, R.y1, R.x1, R.y1 }.ExtendBy(UIRect::UniformBorder(1));
		auto firstChild = GetTree()->GetFirstChild(item);
		if (true && firstChild.IsValid()) // TODO if expanded
		{
			// first child
			_dragTargetLoc = firstChild;
			_dragTargetInsDir = TreeInsertMode::Before;
			//_dragTargetLine.x0 += (level + 1) * 12;
		}
		else
		{
#if 0
			int ll = level - 1;
			auto* PP = N;
			auto* P = N->parent;
			bool foundplace = false;
			for (; P; P = P->parent, ll--)
			{
				for (size_t i = 0; i < P->children.size(); i++)
				{
					if (P->children[i] == PP && i + 1 < P->children.size())
					{
						dragTargetArr = &P->children;
						dragTargetCont = P;
						dragTargetInsertBefore = i + 1;
						_dragTargetLine.x0 += ll * 12;
						foundplace = true;
						break;
					}
				}
				if (foundplace)
					break;
				PP = P;
			}
			if (!foundplace)
			{
				for (size_t i = 0; i < rootNodes.size(); i++)
				{
					if (rootNodes[i] == PP)
					{
						dragTargetArr = &rootNodes;
						dragTargetCont = nullptr;
						dragTargetInsertBefore = i + 1;
						foundplace = true;
						break;
					}
				}
			}
#endif
		}
	}
	else
	{
		// in
		_dragTargetLine = UIRect{ R.x0 + level * 12, lerp(R.y0, R.y1, 0.25f), R.x1, lerp(R.y0, R.y1, 0.75f) };
		_dragTargetLoc = item;
		_dragTargetInsDir = TreeInsertMode::Inside;
	}
}

void TreeEditor::_OnDragDrop(TreeDragData* tdd)
{
	assert(tdd->nodes.size() == 1);
	auto dest = _dragTargetLoc;

	size_t count = tdd->nodes.size();
	GetTree()->RemoveChildrenFromListOf(tdd->nodes.data(), count, dest);
	if (count == 0)
	{
		_dragTargetLoc = {};
		return;
	}

	auto src = tdd->nodes[0];
	auto insDir = _dragTargetInsDir;
	if ((GetTree()->GetFeatureFlags() & ITree::HasChildArray) &&
		src.cont == dest.cont)
	{
		size_t realDest = dest.index + (insDir == TreeInsertMode::After ? 1 : 0);
		if (realDest > src.index)
			dest.index--;
	}
	GetTree()->MoveTo(src, dest, insDir);

	_dragTargetLoc = {};
	_OnEdit(this);
}

} // ui
