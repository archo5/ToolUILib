
#include "SequenceEditor.h"

#include "../Model/Menu.h"


namespace ui {

void SequenceDragData::Render(UIContainer* ctx)
{
	if (scope->itemUICallback)
	{
		auto* seq = scope->GetSequence();
		std::unique_ptr<ISequenceIterator> it{ seq->GetIterator(at) };
		scope->itemUICallback(ctx, scope, it.get());
	}
	else
	{
		ctx->Text("item");
	}
}


SequenceItemElement::SequenceItemElement()
{
	SetFlag(UIObject_DB_Draggable, true);
	auto s = GetStyle();
	s.SetLayout(style::layouts::StackExpand());
	s.SetPadding(0, 0, 0, 16);
}

void SequenceItemElement::OnEvent(UIEvent& e)
{
	Selectable::OnEvent(e);

	if (e.type == UIEventType::ContextMenu)
		ContextMenu();

	if (e.context->DragCheck(e, UIMouseButton::Left))
	{
		ui::DragDrop::SetData(new SequenceDragData(seqEd, num));
		e.context->ReleaseMouse();
	}
	else if (e.type == UIEventType::DragEnter)
	{
		if (auto* ddd = ui::DragDrop::GetData<SequenceDragData>())
		{
			if (ddd->scope == seqEd)
			{
				if (ddd->at != num)
				{
					e.context->MoveClickTo(e.current);
				}
				seqEd->GetSequence()->MoveElementTo(ddd->at, num);
				ddd->at = num;
			}
		}
		RerenderNode();
	}
	else if (e.type == UIEventType::DragLeave || e.type == UIEventType::DragEnd)
	{
		RerenderNode();
	}
}

void SequenceItemElement::ContextMenu()
{
	auto* seq = seqEd->GetSequence();
	bool canInsertOne = seq->GetCurrentSize() < seq->GetSizeLimit();

	auto& CM = ContextMenu::Get();
	CM.Add("Duplicate", !canInsertOne) = [this]() { seqEd->GetSequence()->Duplicate(num); RerenderNode(); };
	CM.Add("Remove") = [this]() { seqEd->GetSequence()->Remove(num); RerenderNode(); };
}

void SequenceItemElement::Init(SequenceEditor* se, size_t n)
{
	seqEd = se;
	num = n;

	bool dragging = false;
	if (auto* dd = ui::DragDrop::GetData<SequenceDragData>())
		if (dd->at == n && dd->scope == se)
			dragging = true;

	Selectable::Init(dragging);
	SetFlag(UIObject_NoPaint, dragging);
}


void SequenceEditor::Render(UIContainer* ctx)
{
	ctx->Push<ListBox>();

	for (std::unique_ptr<ISequenceIterator> it(_sequence->GetIterator()); !_sequence->AtEnd(it.get()); _sequence->Advance(it.get()))
	{
		size_t num = _sequence->GetOffset(it.get());

		ctx->Push<SequenceItemElement>()->Init(this, num);

		OnBuildItem(ctx, it.get());

		if (showDeleteButton)
			OnBuildDeleteButton(ctx, it.get());

		ctx->Pop();
	}

	ctx->Pop();
}

void SequenceEditor::OnBuildItem(UIContainer* ctx, ISequenceIterator* it)
{
	if (itemUICallback)
		itemUICallback(ctx, this, it);
}

void SequenceEditor::OnBuildDeleteButton(UIContainer* ctx, ISequenceIterator* it)
{
	size_t num = GetSequence()->GetOffset(it);
	auto& delBtn = *ctx->MakeWithText<ui::Button>("X");
	delBtn + ui::Width(20);
	delBtn + ui::EventHandler(UIEventType::Activate, [this, num](UIEvent&) { GetSequence()->Remove(num); Rerender(); });
}

SequenceEditor& SequenceEditor::SetSequence(ISequence* s)
{
	_sequence = s;
	return *this;
}

} // ui
