
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

	SetPadding(16, 0, 0, 0);

	seqEd = nullptr;
	num = 0;
}

static bool IsInnerMove(SequenceEditor* seqEd, SequenceDragData* ddd)
{
	bool innermove = seqEd->dragSupport >= SequenceElementDragSupport::SameSequenceEditor && ddd->scope == seqEd;
	innermove |= seqEd->dragSupport >= SequenceElementDragSupport::SameContainer && ddd->scope->_sequence->AreContainersEqual(seqEd->_sequence);
	return innermove;
}

static bool IsCrossMove(SequenceEditor* seqEd, SequenceDragData* ddd)
{
	if (seqEd->dragSupport >= SequenceElementDragSupport::SameType && ddd->scope->_sequence->AreElementTypesEqual(seqEd->_sequence))
	{
		if (seqEd->sameTypeDragParentCheck)
		{
			void* itemptr = nullptr;
			ddd->scope->_sequence->IterateElements(ddd->at, [&itemptr](size_t idx, void* ptr)
			{
				itemptr = ptr;
				return false;
			});
			if (!itemptr) // means the array is somehow already too small to contain the element
				return false;
			return !seqEd->sameTypeDragParentCheck->SEPC_IsThisContainedBy(itemptr);
		}
		// no checks, assume it's fine
		return true;
	}
	return false;
}

void SequenceItemElement::OnEvent(Event& e)
{
	Selectable::OnEvent(e);

	if (e.type == EventType::ContextMenu)
		ContextMenu();

	if (seqEd->dragSupport > SequenceElementDragSupport::None && e.target == e.current && e.context->DragCheck(e, MouseButton::Left))
	{
		DragDrop::SetData(new SequenceDragData(seqEd, GetContentRect().GetWidth(), num));
		e.context->SetKeyboardFocus(nullptr);
		e.context->ReleaseMouse();
	}
	else if (e.type == EventType::DragMove)
	{
		if (auto* ddd = DragDrop::GetData<SequenceDragData>())
		{
			bool validmove = IsInnerMove(seqEd, ddd) || IsCrossMove(seqEd, ddd);
			if (validmove)
			{
				auto r = GetFinalRect();
				bool after = e.position.y > (r.y0 + r.y1) * 0.5f;
				seqEd->_dragTargetPos = num + after;
				float y = after ? r.y1 : r.y0;
				seqEd->_dragTargetLine = UIRect{ r.x0, y, r.x1, y };
			}
		}
	}
	if (e.type == EventType::DragDrop && seqEd->dragSupport > SequenceElementDragSupport::None && seqEd->_dragTargetPos != SIZE_MAX)
	{
		if (auto* ddd = DragDrop::GetData<SequenceDragData>())
		{
			if (IsInnerMove(seqEd, ddd))
			{
				size_t tgt = seqEd->_dragTargetPos - (seqEd->_dragTargetPos > ddd->at);
				seqEd->GetSequence()->MoveElementTo(ddd->at, tgt);
				seqEd->_OnEdit(this);
				if (ddd->scope != seqEd)
					ddd->scope->_OnEdit(ddd->scope);
			}
			else if (IsCrossMove(seqEd, ddd))
			{
				assert(!ddd->scope->GetSequence()->AreContainersEqual(seqEd->GetSequence()));
				size_t tgt = seqEd->_dragTargetPos;
				seqEd->GetSequence()->MoveElementFromOtherSeq(tgt, ddd->scope->GetSequence(), ddd->at);
				seqEd->_OnEdit(this);
				ddd->scope->_OnEdit(ddd->scope);
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
	if (se->dragSupport > SequenceElementDragSupport::None)
		SetFlag(UIObject_DB_Draggable, true);
}


void SequenceEditor::Build()
{
	if (buildFrame)
	{
		Push<ListBoxFrame>();
	}
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
	if (buildFrame)
	{
		Pop();
	}
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
		auto r = _dragTargetLine.ExtendBy(1);
		draw::RectCol(r.x0, r.y0, r.x1, r.y1, Color4f(0.1f, 0.7f, 0.9f, 0.6f));
	}
}

void SequenceEditor::OnReset()
{
	Buildable::OnReset();

	itemUICallback = {};
	itemLayoutPreset = EditorItemContentsLayoutPreset::StackExpandLTRWithDeleteButton;
	allowDelete = true;
	allowDuplicate = true;
	dragSupport = SequenceElementDragSupport::SameSequenceEditor;
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
