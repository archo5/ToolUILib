
#include "SequenceEditor.h"

#include "../Model/Layout.h"
#include "../Model/Menu.h"


namespace ui {

void SequenceDragData::Build()
{
	if (scope->itemUICallback)
	{
		Push<SizeConstraintElement>().SetWidth(width);
		Push<StackTopDownLayoutElement>();
		scope->GetSequence()->IterateElements(at, [this](size_t idx, void* ptr)
		{
			scope->itemUICallback(scope, idx, ptr);
			return false;
		});
		Pop();
		Pop();
	}
	else
	{
		Text("item");
	}
}


void SequenceItemElement::OnReset()
{
	Selectable::OnReset();

	SetFlag(UIObject_DB_Draggable, true);
	SetPadding(16, 0, 0, 0);

	seqEd = nullptr;
	num = 0;
}

void SequenceItemElement::OnEvent(Event& e)
{
	Selectable::OnEvent(e);

	if (e.type == EventType::ContextMenu)
		ContextMenu();

	if (e.context->DragCheck(e, MouseButton::Left))
	{
		DragDrop::SetData(new SequenceDragData(seqEd, GetContentRect().GetWidth(), num));
		e.context->SetKeyboardFocus(nullptr);
		e.context->ReleaseMouse();
	}
	else if (e.type == EventType::DragMove)
	{
		if (auto* ddd = DragDrop::GetData<SequenceDragData>())
		{
			if (ddd->scope == seqEd)
			{
				auto r = GetFinalRect();
				bool after = e.position.y > (r.y0 + r.y1) * 0.5f;
				seqEd->_dragTargetPos = num + after;
				float y = after ? r.y1 : r.y0;
				seqEd->_dragTargetLine = UIRect{ r.x0, y, r.x1, y };
			}
		}
	}
	if (e.type == EventType::DragDrop)
	{
		if (auto* ddd = DragDrop::GetData<SequenceDragData>())
		{
			if (ddd->scope == seqEd)
			{
				size_t tgt = seqEd->_dragTargetPos - (seqEd->_dragTargetPos > ddd->at);
				seqEd->GetSequence()->MoveElementTo(ddd->at, tgt);
				seqEd->_OnEdit(this);
			}
		}
	}
	else if (e.type == EventType::DragLeave)
	{
		if (auto* p = FindParentOfType<SequenceEditor>())
			p->_dragTargetPos = SIZE_MAX;
	}

	if (seqEd->_selImpl.OnEvent(e, seqEd->GetSelectionStorage(), num, true, true))
	{
		Event selev(e.context, this, EventType::SelectionChange);
		e.context->BubblingEvent(selev);
		Rebuild();
	}
}

void SequenceItemElement::ContextMenu()
{
	auto* seq = seqEd->GetSequence();
	bool canInsertOne = seq->GetCurrentSize() < seq->GetSizeLimit();

	auto& CM = ContextMenu::Get();

	if (seqEd->allowDuplicate)
		CM.Add("Duplicate", !canInsertOne) = [this]() { seqEd->GetSequence()->Duplicate(num); seqEd->_OnEdit(this); };

	if (seqEd->allowDelete)
		CM.Add("Remove") = [this]() { seqEd->_OnDelete(this); };

	if (auto* cms = seqEd->GetContextMenuSource())
		cms->FillItemContextMenu(CM, num, 0);
}

void SequenceItemElement::Init(SequenceEditor* se, size_t n)
{
	seqEd = se;
	num = n;

	bool dragging = false;
	if (auto* dd = DragDrop::GetData<SequenceDragData>())
		if (dd->at == n && dd->scope == se)
			dragging = true;

	bool isSel = false;
	if (auto* sel = se->GetSelectionStorage())
		isSel = sel->GetSelectionState(n);

	Selectable::Init(isSel || dragging);
	SetFlag(UIObject_NoPaint, dragging);
}


void SequenceEditor::Build()
{
	Push<ListBoxFrame>();
	Push<StackTopDownLayoutElement>();

	_sequence->IterateElements(0, [this](size_t idx, void* ptr)
	{
		Push<SequenceItemElement>().Init(this, idx);

		if ((itemLayoutPreset & EditorItemContentsLayoutPreset::MASK) == EditorItemContentsLayoutPreset::StackExpandLTR)
			Push<StackExpandLTRLayoutElement>();

		OnBuildItem(idx, ptr);

		if (allowDelete &&
			(itemLayoutPreset & EditorItemContentsLayoutPreset::DeleteButton) == EditorItemContentsLayoutPreset::DeleteButton)
			OnBuildDeleteButton();

		if ((itemLayoutPreset & EditorItemContentsLayoutPreset::MASK) == EditorItemContentsLayoutPreset::StackExpandLTR)
			Pop();

		Pop();
		return true;
	});

	Pop();
	Pop();
}

void SequenceEditor::OnEvent(Event& e)
{
	Buildable::OnEvent(e);

	if (e.type == EventType::ContextMenu)
	{
		if (auto* cms = GetContextMenuSource())
			cms->FillListContextMenu(ContextMenu::Get());
	}
}

void SequenceEditor::OnPaint(const UIPaintContext& ctx)
{
	Buildable::OnPaint(ctx);

	if (_dragTargetPos < SIZE_MAX)
	{
		auto r = _dragTargetLine.ExtendBy(UIRect::UniformBorder(1));
		draw::RectCol(r.x0, r.y0, r.x1, r.y1, Color4f(0.1f, 0.7f, 0.9f, 0.6f));
	}
}

void SequenceEditor::OnReset()
{
	Buildable::OnReset();

	itemUICallback = {};
	itemLayoutPreset = EditorItemContentsLayoutPreset::StackExpandLTRWithDeleteButton;
	_sequence = nullptr;
	_selStorage = nullptr;
	_ctxMenuSrc = nullptr;
	_selImpl.OnReset();
}

void SequenceEditor::OnBuildItem(size_t idx, void* ptr)
{
	if (itemUICallback)
		itemUICallback(this, idx, ptr);
}

void SequenceEditor::OnBuildDeleteButton()
{
	Push<SizeConstraintElement>().SetWidth(20);
	auto& delBtn = Push<Button>();
	Make<IconElement>().SetDefaultStyle(DefaultIconStyle::Delete);
	Pop();
	Pop();
	delBtn + AddEventHandler(EventType::Activate, [this, &delBtn](Event& e)
	{
		auto* sie = delBtn.FindParentOfType<SequenceItemElement>();
		_OnDelete(sie);
	});
}

SequenceEditor& SequenceEditor::SetSequence(ISequence* s)
{
	_sequence = s;
	return *this;
}

SequenceEditor& SequenceEditor::SetSelectionStorage(ISelectionStorage* s)
{
	_selStorage = s;
	return *this;
}

SequenceEditor& SequenceEditor::SetContextMenuSource(IListContextMenuSource* src)
{
	_ctxMenuSrc = src;
	return *this;
}

SequenceEditor& SequenceEditor::SetSelectionMode(SelectionMode mode)
{
	_selImpl.selectionMode = mode;
	return *this;
}

void SequenceEditor::_OnEdit(UIObject* who)
{
	system->eventSystem.OnChange(who);
	system->eventSystem.OnCommit(who);
	who->Rebuild();
}

void SequenceEditor::_OnDelete(SequenceItemElement* sie)
{
	if (!allowDelete)
		return; // last line of defense

	auto ear = EditorActionResponse::Proceed;
	if (onBeforeRemoveElement)
		ear = onBeforeRemoveElement(sie->num);

	if (ear == EditorActionResponse::Proceed)
		GetSequence()->Remove(sie->num);

	if (ear != EditorActionResponse::Stop)
		_OnEdit(sie);
}

} // ui
