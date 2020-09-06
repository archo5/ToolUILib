
#include "SequenceEditor.h"

#include "Menu.h"


namespace ui {

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

	if (e.type == UIEventType::Click && e.GetButton() == UIMouseButton::Right)
		ContextMenu();

	if (e.context->DragCheck(e, UIMouseButton::Left))
	{
		ui::DragDrop::SetData(new SequenceDragData(seqEd, num));
		e.context->ReleaseMouse();
	}
	else if (e.type == UIEventType::DragEnter)
	{
		if (auto* ddd = static_cast<SequenceDragData*>(ui::DragDrop::GetData(SequenceDragData::Name)))
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
	MenuItem items[] =
	{
		ui::MenuItem("Duplicate", {}, !canInsertOne).Func([this]() { seqEd->GetSequence()->Duplicate(num); RerenderNode(); }),
		ui::MenuItem("Remove").Func([this]() { seqEd->GetSequence()->Remove(num); RerenderNode(); }),
	};
	ui::Menu menu(items);
	menu.Show(this);
}

void SequenceItemElement::Init(SequenceEditor* se, size_t n)
{
	seqEd = se;
	num = n;

	bool dragging = false;
	if (auto* dd = static_cast<SequenceDragData*>(ui::DragDrop::GetData(SequenceDragData::Name)))
		if (dd->at == n && dd->scope == se)
			dragging = true;

	Selectable::Init(dragging);
}


void SequenceEditor::Render(UIContainer* ctx)
{
	ctx->Push<ListBox>();

	for (std::unique_ptr<ISequenceIterator> it(_sequence->GetIterator()); !_sequence->AtEnd(it.get()); _sequence->Advance(it.get()))
	{
		size_t num = _sequence->GetOffset(it.get());

		ctx->Push<SequenceItemElement>()->Init(this, num);

		OnBuildItem(ctx, it.get());

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
