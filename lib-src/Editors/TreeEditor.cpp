
#include "TreeEditor.h"

#include "../Model/Menu.h"
#include "../Model/Layout.h"


namespace ui {

void TreeItemElement::OnReset()
{
	Selectable::OnReset();

	SetFlag(UIObject_DB_Draggable, true);
	SetPadding(16, 0, 0, 0);

	treeEd = nullptr;
	path = {};
}

void TreeItemElement::OnEvent(Event& e)
{
	Selectable::OnEvent(e);

	if (e.type == EventType::ContextMenu)
		ContextMenu();

	if (e.context->DragCheck(e, MouseButton::Left))
	{
		DragDrop::SetData(new TreeDragData(treeEd, { path }));
		e.context->SetKeyboardFocus(nullptr);
		e.context->ReleaseMouse();
	}
	else if (e.type == EventType::DragMove)
	{
		if (auto* ddd = DragDrop::GetData<TreeDragData>())
			if (ddd->scope == treeEd)
				treeEd->_OnDragMove(ddd, path, GetFinalRect(), e);
	}
	if (e.type == EventType::DragDrop)
	{
		if (auto* ddd = DragDrop::GetData<TreeDragData>())
			if (ddd->scope == treeEd)
				treeEd->_OnDragDrop(ddd);
	}
	else if (e.type == EventType::DragLeave)
	{
		if (auto* ddd = DragDrop::GetData<TreeDragData>())
			if (ddd->scope == treeEd)
				treeEd->_dragTargetLoc = {};
	}

#if 1 // TODO
	if (e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Left)
	{
		treeEd->GetTree()->ClearSelection();
		treeEd->GetTree()->SetSelectionState(path, true);
		Event selev(e.context, this, EventType::SelectionChange);
		e.context->BubblingEvent(selev);
		Rebuild();
	}
#else
	if (treeEd->_selImpl.OnEvent(e, treeEd->GetSelectionStorage(), num, true, true))
	{
		Event selev(e.context, this, EventType::SelectionChange);
		e.context->BubblingEvent(selev);
		Rebuild();
	}
#endif
}

void TreeItemElement::ContextMenu()
{
	auto& CM = ContextMenu::Get();
	CM.Add("Duplicate") = [this]() { treeEd->GetTree()->Duplicate(path); treeEd->_OnEdit(this); };
	CM.Add("Remove") = [this]() { treeEd->GetTree()->Remove(path); treeEd->_OnEdit(this); };
	treeEd->GetTree()->FillItemContextMenu(CM, path);
}

void TreeItemElement::Init(TreeEditor* te, const TreePath& p)
{
	treeEd = te;
	path = p;

	bool dragging = false;
	if (auto* dd = DragDrop::GetData<TreeDragData>())
		if (dd->paths.Size() == 1 && dd->paths[0] == p && dd->scope == te)
			dragging = true;

	Selectable::Init(dragging || te->GetTree()->GetSelectionState(path));
}


void TreeEditor::Build()
{
	Push<ListBoxFrame>();
	Push<SizeConstraintElement>().SetMinHeight(22);
	Push<StackTopDownLayoutElement>();

	TreePath path;
	OnBuildList(path);

	Pop();
	Pop();
	Pop();
}

void TreeEditor::OnEvent(Event& e)
{
	Buildable::OnEvent(e);

	if (e.type == EventType::ContextMenu)
	{
		GetTree()->FillListContextMenu(ContextMenu::Get());
	}
}

void TreeEditor::OnPaint(const UIPaintContext& ctx)
{
	Buildable::OnPaint(ctx);

	if (_dragTargetLoc.NotEmpty())
	{
		auto r = _dragTargetLine.ExtendBy(UIRect::UniformBorder(1));
		draw::RectCol(r.x0, r.y0, r.x1, r.y1, Color4f(0.1f, 0.7f, 0.9f, 0.6f));
	}
}

void TreeEditor::OnReset()
{
	Buildable::OnReset();

	itemUICallback = {};
	itemLayoutPreset = EditorItemContentsLayoutPreset::StackExpandLTRWithDeleteButton;
	_tree = nullptr;
	_selImpl.OnReset();
}

void TreeEditor::OnBuildChildList(TreePath& path)
{
	Push<PaddingElement>().SetPaddingLeft(8);
	Push<StackTopDownLayoutElement>();

	OnBuildList(path);

	Pop();
	Pop();
}

void TreeEditor::OnBuildList(TreePath& path)
{
	size_t index = 0;
	_tree->IterateChildren(path, [this, &index, &path](void* data)
	{
		path.Append(index);

		Push<TreeItemElement>().Init(this, path);

		if ((itemLayoutPreset & EditorItemContentsLayoutPreset::MASK) == EditorItemContentsLayoutPreset::StackExpandLTR)
			Push<StackExpandLTRLayoutElement>();

		OnBuildItem(path, data);

		if ((itemLayoutPreset & EditorItemContentsLayoutPreset::DeleteButton) == EditorItemContentsLayoutPreset::DeleteButton)
			OnBuildDeleteButton();

		if ((itemLayoutPreset & EditorItemContentsLayoutPreset::MASK) == EditorItemContentsLayoutPreset::StackExpandLTR)
			Pop();

		Pop();

		if (_tree->HasChildren(path))
		{
			OnBuildChildList(path);
		}

		path.RemoveLast();
		index++;
	});
}

void TreeEditor::OnBuildItem(TreePathRef path, void* data)
{
	if (itemUICallback)
		itemUICallback(this, path, data);
}

void TreeEditor::OnBuildDeleteButton()
{
	Push<SizeConstraintElement>().SetWidth(20);
	auto& delBtn = MakeWithText<Button>("X");
	Pop();
	delBtn + AddEventHandler(EventType::Activate, [this, &delBtn](Event&)
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

TreeEditor& TreeEditor::SetSelectionMode(SelectionMode mode)
{
	_selImpl.selectionMode = mode;
	return *this;
}

void TreeEditor::_OnEdit(UIObject* who)
{
	system->eventSystem.OnChange(who);
	system->eventSystem.OnCommit(who);
	who->Rebuild();
}

void TreeEditor::_OnDragMove(TreeDragData* tdd, TreePathRef hoverPath, const UIRect& rect, Event& e)
{
	auto& R = rect;
	if (e.position.y < lerp(R.y0, R.y1, 0.25f))
	{
		// above
		_dragTargetLine = UIRect{ R.x0, R.y0, R.x1, R.y0 };
		_dragTargetLoc = hoverPath;
	}
	else if (e.position.y > lerp(R.y0, R.y1, 0.75f))
	{
		// below
		_dragTargetLine = UIRect{ R.x0, R.y1, R.x1, R.y1 };
		_dragTargetLoc = hoverPath;
		if (true && GetTree()->HasChildren(hoverPath)) // TODO if expanded
		{
			// first child
			_dragTargetLoc.Append(0);
		}
		else
		{
			auto PP = hoverPath;
			auto P = hoverPath.without_last();
			bool foundplace = false;
			for (;;)
			{
				if (PP.last() + 1 < GetTree()->GetChildCount(P))
				{
					_dragTargetLoc = P;
					_dragTargetLoc.Append(PP.last() + 1);
					foundplace = true;
					break;
				}
				if (P.empty())
					break;
				PP = P;
				P = P.without_last();
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
		_dragTargetLoc = hoverPath;
		_dragTargetLoc.Append(GetTree()->GetChildCount(hoverPath));
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

static void RemoveUsingPrefix(Array<TreePathRef>& paths, TreePathRef prefix)
{
	for (size_t i = 0; i < paths.size(); i++)
	{
		if (PathMatchesPrefix(paths[i], prefix))
		{
			paths.UnorderedRemoveAt(i--);
		}
	}
}

void TreeEditor::_OnDragDrop(TreeDragData* tdd)
{
	assert(tdd->paths.size() == 1);
	auto dest = _dragTargetLoc;

	Array<TreePathRef> refs;
	refs.Reserve(tdd->paths.size());
	for (auto& path : tdd->paths)
	{
		if (PathMatchesPrefix(dest, path))
		{
			// destination is inside one of the selected items
			_dragTargetLoc = {};
			return;
		}
		refs.Append(path);
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
