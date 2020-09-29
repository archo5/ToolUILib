
#include "SequenceEditor.h"

#include "../Model/Menu.h"


namespace ui {

void SequenceDragData::Render(UIContainer* ctx)
{
	if (scope->itemUICallback)
	{
		auto* seq = scope->GetSequence();
		std::unique_ptr<ISequenceIterator> it{ seq->GetIterator(at) };
		ctx->PushBox() + Width(width);
		scope->itemUICallback(ctx, scope, it.get());
		ctx->Pop();
	}
	else
	{
		ctx->Text("item");
	}
}


void SequenceItemElement::OnInit()
{
	Selectable::OnInit();
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
		ui::DragDrop::SetData(new SequenceDragData(seqEd, GetContentRect().GetWidth(), num));
		e.context->ReleaseMouse();
	}
	else if (e.type == UIEventType::DragMove)
	{
		if (auto* ddd = ui::DragDrop::GetData<SequenceDragData>())
		{
			if (ddd->scope == seqEd)
			{
				if (auto* p = FindParentOfType<SequenceEditor>())
				{
					auto r = GetBorderRect();
					bool after = e.y > (r.y0 + r.y1) * 0.5f;
					p->_dragTargetPos = num + after;
					float y = after ? r.y1 : r.y0;
					p->_dragTargetLine = UIRect{ r.x0, y, r.x1, y }.ExtendBy(UIRect::UniformBorder(1));
				}
			}
		}
	}
	if (e.type == UIEventType::DragDrop)
	{
		if (auto* ddd = ui::DragDrop::GetData<SequenceDragData>())
		{
			if (ddd->scope == seqEd)
			{
				if (auto* p = FindParentOfType<SequenceEditor>())
				{
					size_t tgt = p->_dragTargetPos - (p->_dragTargetPos > ddd->at);
					seqEd->GetSequence()->MoveElementTo(ddd->at, tgt);
					RerenderNode();
				}
			}
		}
	}
	else if (e.type == UIEventType::DragLeave)
	{
		if (auto* p = FindParentOfType<SequenceEditor>())
			p->_dragTargetPos = SIZE_MAX;
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

void SequenceEditor::OnPaint()
{
	Node::OnPaint();

	if (_dragTargetPos < SIZE_MAX)
	{
		auto r = _dragTargetLine;
		ui::draw::RectCol(r.x0, r.y0, r.x1, r.y1, ui::Color4f(0.1f, 0.7f, 0.9f, 0.6f));
	}
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
