
#include "TreeEditor.h"

#include "../Model/Menu.h"

#include <algorithm>


namespace ui {

#if 0
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
#endif


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
		ui::DragDrop::SetData(new TreeDragData(treeEd, { path }));
		e.context->SetKeyboardFocus(nullptr);
		e.context->ReleaseMouse();
	}
	else if (e.type == UIEventType::DragMove)
	{
		if (auto* ddd = ui::DragDrop::GetData<TreeDragData>())
			if (ddd->scope == treeEd)
				treeEd->_OnDragMove(ddd, path, GetBorderRect(), e);
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

void TreeItemElement::Init(TreeEditor* te, const TreePath& p)
{
	treeEd = te;
	path = p;

	bool dragging = false;
	if (auto* dd = ui::DragDrop::GetData<TreeDragData>())
		if (dd->paths.size() == 1 && dd->paths[0] == p && dd->scope == te)
			dragging = true;

	Selectable::Init(dragging);
}


void TreeEditor::Render(UIContainer* ctx)
{
	auto s = ctx->Push<ListBox>()->GetStyle();
	s.SetMinHeight(22);
	s.SetBoxSizing(style::BoxSizing::ContentBox);

	TreePath path;
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

	if (!_dragTargetLoc.empty())
	{
		auto r = _dragTargetLine.ExtendBy(UIRect::UniformBorder(1));
		ui::draw::RectCol(r.x0, r.y0, r.x1, r.y1, ui::Color4f(0.1f, 0.7f, 0.9f, 0.6f));
	}
}

void TreeEditor::OnSerialize(IDataSerializer& s)
{
}

void TreeEditor::OnBuildChildList(UIContainer* ctx, ItemLoc firstChild, TreePath& path)
{
	ctx->PushBox().GetStyle().SetPaddingLeft(8);

	OnBuildList(ctx, firstChild, path);

	ctx->Pop();
}

void TreeEditor::OnBuildList(UIContainer* ctx, ItemLoc firstNode, TreePath& path)
{
	for (auto node = firstNode; !_tree->AtEnd(node); node = _tree->GetNext(node))
	{
		path.push_back(node.index);

		ctx->Push<TreeItemElement>()->Init(this, path);

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

void TreeEditor::OnBuildItem(UIContainer* ctx, ItemLoc node, TreePath& path)
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

void TreeEditor::_OnDragMove(TreeDragData* tdd, TreePathRef hoverPath, const UIRect& rect, UIEvent& e)
{
	auto& R = rect;
	if (e.y < lerp(R.y0, R.y1, 0.25f))
	{
		// above
		_dragTargetLine = UIRect{ R.x0, R.y0, R.x1, R.y0 };
		_dragTargetLoc.assign(hoverPath.begin(), hoverPath.end());
	}
	else if (e.y > lerp(R.y0, R.y1, 0.75f))
	{
		// below
		_dragTargetLine = UIRect{ R.x0, R.y1, R.x1, R.y1 };
		_dragTargetLoc.assign(hoverPath.begin(), hoverPath.end());
		if (true && GetTree()->GetChildCount(hoverPath)) // TODO if expanded
		{
			// first child
			_dragTargetLoc.push_back(0);
		}
		else
		{
			auto PP = hoverPath;
			auto P = hoverPath.without_last();
			bool foundplace = false;
			for (; !P.empty(); PP = P, P = P.without_last())
			{
				if (PP.last() + 1 < GetTree()->GetChildCount(P))
				{
					_dragTargetLoc.assign(P.begin(), P.end());
					_dragTargetLoc.push_back(PP.last() + 1);
					foundplace = true;
					break;
				}
			}
			if (!foundplace)
			{
				_dragTargetLoc = { GetTree()->GetChildCount({}) };
			}
		}
	}
	else
	{
		// in
		_dragTargetLine = UIRect{ R.x0, lerp(R.y0, R.y1, 0.25f), R.x1, lerp(R.y0, R.y1, 0.75f) };
		_dragTargetLoc.assign(hoverPath.begin(), hoverPath.end());
		_dragTargetLoc.push_back(GetTree()->GetChildCount(hoverPath));
	}
}

static bool PathMatchesPrefix(TreePathRef path, TreePathRef prefix)
{
	if (path.size() < prefix.size())
		return false;
	for (size_t i = 0; i < prefix.size(); i++)
	{
		if (path[i] != prefix[i])
			return false;
	}
	return true;
}

static void RemoveUsingPrefix(std::vector<TreePathRef>& paths, TreePathRef prefix)
{
	for (size_t i = 0; i < paths.size(); i++)
	{
		if (PathMatchesPrefix(paths[i], prefix))
		{
			paths[i--] = paths.back();
			paths.pop_back();
		}
	}
}

void TreeEditor::_OnDragDrop(TreeDragData* tdd)
{
	assert(tdd->paths.size() == 1);
	auto dest = _dragTargetLoc;

	std::vector<TreePathRef> refs;
	refs.reserve(tdd->paths.size());
	for (auto& path : tdd->paths)
	{
		if (PathMatchesPrefix(dest, path))
		{
			// destination is inside one of the selected items
			_dragTargetLoc = {};
			return;
		}
		refs.push_back(path);
	}

	auto src = tdd->paths[0];
	if (dest.size() >= src.size() &&
		PathMatchesPrefix(dest, TreePathRef(src).without_last()) &&
		dest[src.size() - 1] > src[src.size() - 1])
		dest[src.size() - 1]--;
	GetTree()->MoveTo(src, dest);

	_dragTargetLoc = {};
	_OnEdit(this);
}

} // ui
